use crate::{Config, RequestHandler};
use bytesize::ByteSize;
use failure::Error;
use futures::sink::SinkExt;
use futures::stream::StreamExt;
use toku_connection::handler::{DelegatedFrame, Handler, Ready};
use toku_connection::{find_encoding, ReaderWriter};
use toku_connection::{IdSequence, TokuError};
use toku_protocol::frames::{Frame, Hello, HelloAck, TokuFrame, Push, Request, Response};
use toku_protocol::upgrade::{Codec, UpgradeFrame};
use toku_protocol::VERSION;
use std::future::Future;
use std::pin::Pin;
use std::sync::Arc;
use std::time::Duration;
use tokio::net::TcpStream;
use tokio::task::spawn;
use tokio_util::codec::Framed;

pub struct ConnectionHandler<R: RequestHandler> {
    config: Arc<Config<R>>,
}

impl<R: RequestHandler> ConnectionHandler<R> {
    pub fn new(config: Arc<Config<R>>) -> Self {
        Self { config }
    }
}

impl<R: RequestHandler> Handler for ConnectionHandler<R> {
    // Server doesn't have any internal events.
    type InternalEvent = ();

    const SEND_GO_AWAY: bool = true;

    fn max_payload_size(&self) -> ByteSize {
        self.config.max_payload_size
    }

    fn upgrade(
        &self,
        tcp_stream: TcpStream,
    ) -> Pin<Box<dyn Future<Output = Result<TcpStream, Error>> + Send>> {
        let max_payload_size = self.max_payload_size();
        let framed_socket = Framed::new(tcp_stream, Codec::new(max_payload_size));
        let (mut writer, mut reader) = framed_socket.split();

        Box::pin(async move {
            match reader.next().await {
                Some(Ok(UpgradeFrame::Request)) => {
                    if let Err(e) = writer.send(UpgradeFrame::Response).await {
                        return Err(e);
                    }
                    Ok(writer.reunite(reader)?.into_inner())
                }
                Some(Ok(frame)) => Err(TokuError::InvalidUpgradeFrame { frame }.into()),
                Some(Err(e)) => Err(e),
                None => Err(TokuError::TcpStreamClosed.into()),
            }
        })
    }

    fn handshake(
        &mut self,
        mut reader_writer: ReaderWriter,
    ) -> Pin<
        Box<
            dyn Future<Output = Result<(Ready, ReaderWriter), (Error, Option<ReaderWriter>)>>
                + Send,
        >,
    > {
        let ping_interval = self.config.ping_interval;
        let supported_encodings = self.config.supported_encodings;
        Box::pin(async move {
            match reader_writer.reader.next().await {
                Some(Ok(frame)) => {
                    match Self::handle_handshake_frame(frame, ping_interval, supported_encodings) {
                        Ok((ready, hello_ack)) => {
                            reader_writer = match reader_writer.write(hello_ack).await {
                                Ok(reader_writer) => reader_writer,
                                Err(e) => return Err((e.into(), None)),
                            };
                            Ok((ready, reader_writer))
                        }
                        Err(e) => Err((e, Some(reader_writer))),
                    }
                }
                Some(Err(e)) => Err((e, Some(reader_writer))),
                None => Err((TokuError::TcpStreamClosed.into(), Some(reader_writer))),
            }
        })
    }

    fn handle_frame(
        &mut self,
        frame: DelegatedFrame,
        encoding: &'static str,
    ) -> Option<Pin<Box<dyn Future<Output = Result<Response, (Error, u32)>> + Send>>> {
        match frame {
            DelegatedFrame::Push(push) => {
                spawn(handle_push(self.config.clone(), push, encoding));
                None
            }
            DelegatedFrame::Request(request) => {
                let response_future = handle_request(self.config.clone(), request, encoding);
                Some(Box::pin(response_future))
            }
            DelegatedFrame::Error(_) => None,
            DelegatedFrame::Response(_) => None,
        }
    }

    fn handle_internal_event(
        &mut self,
        _event: (),
        _id_sequence: &mut IdSequence,
    ) -> Option<TokuFrame> {
        None
    }

    fn on_ping_received(&mut self) {}
}
impl<R: RequestHandler> ConnectionHandler<R> {
    fn handle_handshake_frame(
        frame: TokuFrame,
        ping_interval: Duration,
        supported_encodings: &'static [&'static str],
    ) -> Result<(Ready, HelloAck), Error> {
        match frame {
            TokuFrame::Hello(hello) => {
                Self::handle_handshake_hello(hello, ping_interval, supported_encodings)
            }
            TokuFrame::GoAway(go_away) => Err(TokuError::ToldToGoAway { go_away }.into()),
            frame => Err(TokuError::InvalidOpcode {
                actual: frame.opcode(),
                expected: Some(Hello::OPCODE),
            }
            .into()),
        }
    }

    fn handle_handshake_hello(
        hello: Hello,
        ping_interval: Duration,
        supported_encodings: &'static [&'static str],
    ) -> Result<(Ready, HelloAck), Error> {
        let Hello {
            flags,
            version,
            encodings,
            // compression not supported
            compressions: _compressions,
        } = hello;
        if version != VERSION {
            return Err(TokuError::UnsupportedVersion {
                expected: VERSION,
                actual: version,
            }
            .into());
        }
        let encoding = Self::negotiate_encoding(&encodings, supported_encodings)?;
        let hello_ack = HelloAck {
            flags,
            ping_interval_ms: ping_interval.as_millis() as u32,
            encoding: encoding.to_string(),
            compression: None,
        };
        let ready = Ready {
            ping_interval,
            encoding,
        };
        Ok((ready, hello_ack))
    }

    fn negotiate_encoding(
        client_encodings: &[String],
        supported_encodings: &'static [&'static str],
    ) -> Result<&'static str, Error> {
        for client_encoding in client_encodings {
            if let Some(encoding) = find_encoding(client_encoding, supported_encodings) {
                return Ok(encoding);
            }
        }
        Err(TokuError::NoCommonEncoding.into())
    }
}

async fn handle_push<R: RequestHandler>(
    config: Arc<Config<R>>,
    push: Push,
    encoding: &'static str,
) {
    let Push {
        payload,
        flags: _flags,
    } = push;
    config.request_handler.handle_push(payload, encoding).await
}

async fn handle_request<R: RequestHandler>(
    config: Arc<Config<R>>,
    request: Request,
    encoding: &'static str,
) -> Result<Response, (Error, u32)> {
    let Request {
        payload: request_payload,
        flags: _flags,
        sequence_id,
    } = request;
    let response_payload = config
        .request_handler
        .handle_request(request_payload, encoding)
        .await;
    Ok(Response {
        flags: 0,
        sequence_id,
        payload: response_payload,
    })
}
