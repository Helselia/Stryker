use super::connection::Event;
use super::error::TokuError;
use super::handler::{DelegatedFrame, Handler};
use super::id_sequence::IdSequence;
use super::sender::Sender;
use crate::TokuErrorCode;
use failure::Error;
use toku_protocol::frames::{Error as ErrorFrame, TokuFrame, Ping, Pong, Response};
use tokio::task::spawn;

/// Main handler of connection `Event`s.
pub struct EventHandler<H: Handler> {
    handler: H,
    pong_received: bool,
    id_sequence: IdSequence,
    self_sender: Sender<H::InternalEvent>,
    encoding: &'static str,
}

/// Standard return type for handler functions.
///
/// Event handler functions return an optional `TokuFrame` that will
/// be sent back over the connection.
type MaybeFrameResult = Result<Option<TokuFrame>, Error>;

impl<H: Handler> EventHandler<H> {
    pub fn new(self_sender: Sender<H::InternalEvent>, handler: H, encoding: &'static str) -> Self {
        Self {
            handler,
            pong_received: true,
            id_sequence: IdSequence::default(),
            self_sender,
            encoding,
        }
    }

    /// High level event handler entry point. This is called by the connection whenever an
    /// event comes in.
    pub fn handle_event(&mut self, event: Event<H::InternalEvent>) -> MaybeFrameResult {
        match event {
            Event::Ping => self.send_ping(),
            Event::SocketReceive(frame) => self.handle_frame(frame),
            Event::InternalEvent(internal_event) => self.handle_internal_event(internal_event),
            Event::ResponseComplete(response) => self.handle_response_complete(response),
            Event::Close => self.handle_close(),
        }
    }

    /// Handles a request to ping the other side. Returns an `Error` if a `Pong` hasn't been
    /// received since the last ping.
    fn send_ping(&mut self) -> MaybeFrameResult {
        if self.pong_received {
            let sequence_id = self.id_sequence.next();
            let ping = Ping {
                sequence_id,
                flags: 0,
            };
            self.pong_received = false;
            Ok(Some(ping.into()))
        } else {
            Err(TokuError::PingTimeout.into())
        }
    }

    /// Handles a frame received from the socket. Delegates some frames to the `ConnectionHandler`.
    /// Optionally returns a `TokuFrame` that will be sent back over the socket.
    fn handle_frame(&mut self, frame: TokuFrame) -> MaybeFrameResult {
        match frame {
            TokuFrame::Hello(_) | TokuFrame::HelloAck(_) => self.handle_handshake_frame(frame),
            TokuFrame::Ping(ping) => self.handle_ping_frame(ping),
            TokuFrame::Pong(pong) => self.handle_pong_frame(pong),
            TokuFrame::Request(request) => self.delegate_frame(request),
            TokuFrame::Response(response) => self.delegate_frame(response),
            TokuFrame::Push(push) => self.delegate_frame(push),
            TokuFrame::GoAway(go_away) => Err(TokuError::ToldToGoAway { go_away }.into()),
            TokuFrame::Error(error) => self.delegate_frame(error),
        }
    }

    /// Handshake should have already completed. This is an error at this point.
    fn handle_handshake_frame(&mut self, frame: TokuFrame) -> MaybeFrameResult {
        Err(TokuError::InvalidOpcode {
            actual: frame.opcode(),
            expected: None,
        }
        .into())
    }

    /// Delegates a frame to the connection handler.
    fn delegate_frame<D: Into<DelegatedFrame>>(&mut self, delegated_frame: D) -> MaybeFrameResult {
        let delegated_frame = delegated_frame.into();
        let maybe_future = self.handler.handle_frame(delegated_frame, self.encoding);
        // If the connection handler returns a future, execute the future async and send it back
        // to the main event loop. The main event loop will send it through the socket.
        if let Some(future) = maybe_future {
            let connection_sender = self.self_sender.clone();
            spawn(async move {
                let response = future.await;
                // It's okay to ignore this result. The connection closed.
                let _result = connection_sender.response_complete(response);
            });
        }
        Ok(None)
    }

    fn handle_ping_frame(&mut self, ping: Ping) -> MaybeFrameResult {
        let pong = Pong {
            flags: ping.flags,
            sequence_id: ping.sequence_id,
        };
        self.handler.on_ping_received();
        Ok(Some(pong.into()))
    }

    fn handle_pong_frame(&mut self, _pong: Pong) -> MaybeFrameResult {
        self.pong_received = true;
        Ok(None)
    }

    /// A response was computed. Send it back over the socket.
    fn handle_response_complete(&self, result: Result<Response, (Error, u32)>) -> MaybeFrameResult {
        match result {
            Ok(response) => Ok(Some(response.into())),
            Err((error, sequence_id)) => {
                let error = ErrorFrame {
                    flags: 0,
                    sequence_id,
                    code: TokuErrorCode::InternalServerError as u16,
                    payload: format!("{:?}", error.to_string()).as_bytes().to_vec(),
                };
                Ok(Some(error.into()))
            }
        }
    }

    fn handle_internal_event(&mut self, internal_event: H::InternalEvent) -> MaybeFrameResult {
        Ok(self
            .handler
            .handle_internal_event(internal_event, &mut self.id_sequence))
    }

    /// Close requested. Return an `Error` to close the connection.
    fn handle_close(&mut self) -> MaybeFrameResult {
        Err(TokuError::ConnectionCloseRequested.into())
    }
}
