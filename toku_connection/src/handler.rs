use crate::framed_io::ReaderWriter;
use crate::id_sequence::IdSequence;
use bytesize::ByteSize;
use failure::Error;
use toku_protocol::frames::{Error as ErrorFrame, TokuFrame, Push, Request, Response};
use std::future::Future;
use std::pin::Pin;
use std::time::Duration;
use tokio::net::TcpStream;

/// Specific types of toku frames that are delegated to a connection handler.  The rest of the
/// frames will be handled by the connection itself.
#[derive(Debug)]
pub enum DelegatedFrame {
    Push(Push),
    Request(Request),
    Response(Response),
    Error(ErrorFrame),
}

/// Settings negotiated from handshake.
#[derive(Debug)]
pub struct Ready {
    pub ping_interval: Duration,
    pub encoding: &'static str,
}

/// A trait that handles the specific functionality of a connection. The client and server each
/// implement this.
pub trait Handler: Send + Sync + 'static {
    /// Events specific to the implementing connection handler. They will be passed through to the
    /// handle_internal_event callback.
    type InternalEvent: Send;
    // Whether or not the connection should send a GoAway frame on close.
    const SEND_GO_AWAY: bool;

    /// The maximum payload size this connection can handle.
    fn max_payload_size(&self) -> ByteSize;
    /// Takes a tcp stream and completes an HTTP upgrade.
    fn upgrade(
        &self,
        tcp_stream: TcpStream,
    ) -> Pin<Box<dyn Future<Output = Result<TcpStream, Error>> + Send>>;
    /// Hello/HelloAck handshake.
    fn handshake(
        &mut self,
        reader_writer: ReaderWriter,
    ) -> Pin<
        Box<
            dyn Future<Output = Result<(Ready, ReaderWriter), (Error, Option<ReaderWriter>)>>
                + Send,
        >,
    >;
    /// Handle a single delegated frame. Optionally returns a future that resolves to a
    /// Response. The Response will be sent back through the socket to the other side.
    fn handle_frame(
        &mut self,
        frame: DelegatedFrame,
        encoding: &'static str,
    ) -> Option<Pin<Box<dyn Future<Output = Result<Response, (Error, u32)>> + Send>>>;
    /// Handle internal events for this connection. Completely opaque to the connection. Optionally
    /// return a `TokuFrame` that will be sent back through the socket to the other side.
    fn handle_internal_event(
        &mut self,
        event: Self::InternalEvent,
        id_sequence: &mut IdSequence,
    ) -> Option<TokuFrame>;
    /// Periodic callback that fires whenever a ping fires.
    fn on_ping_received(&mut self);
}

impl From<Push> for DelegatedFrame {
    fn from(push: Push) -> DelegatedFrame {
        DelegatedFrame::Push(push)
    }
}

impl From<Request> for DelegatedFrame {
    fn from(request: Request) -> DelegatedFrame {
        DelegatedFrame::Request(request)
    }
}

impl From<Response> for DelegatedFrame {
    fn from(response: Response) -> DelegatedFrame {
        DelegatedFrame::Response(response)
    }
}

impl From<ErrorFrame> for DelegatedFrame {
    fn from(error: ErrorFrame) -> DelegatedFrame {
        DelegatedFrame::Error(error)
    }
}
