use failure::{Error, Fail};
use toku_protocol::frames::GoAway;
use toku_protocol::upgrade::UpgradeFrame;

#[derive(Debug, Fail)]
pub enum TokuError {
    #[fail(display = "TCP Connection closed.")]
    TcpStreamClosed,
    #[fail(display = "Connection close requested.")]
    ConnectionCloseRequested,
    #[fail(display = "Connection closed.")]
    ConnectionClosed,
    #[fail(display = "Invalid upgrade frame. frame={:?}", frame)]
    InvalidUpgradeFrame { frame: UpgradeFrame },
    #[fail(display = "Connection not ready.")]
    NotReady,
    #[fail(display = "Told to go away. go_away={:?}", go_away)]
    ToldToGoAway { go_away: GoAway },
    #[fail(
        display = "Invalid Opcode. actual={:?} expected={:?}",
        actual, expected
    )]
    InvalidOpcode { actual: u8, expected: Option<u8> },
    #[fail(
        display = "Unsupported Version. expected={} actual={}",
        expected, actual
    )]
    UnsupportedVersion { expected: u8, actual: u8 },
    #[fail(display = "No common encoding.")]
    NoCommonEncoding,
    #[fail(display = "No common compression.")]
    NoCommonCompression,
    #[fail(display = "Invalid encoding.")]
    InvalidEncoding,
    #[fail(display = "Invalid compression.")]
    InvalidCompression,
    #[fail(display = "Ping timeout.")]
    PingTimeout,
    #[fail(display = "Internal server error. error={:?}", error)]
    InternalServerError { error: Error },
    #[fail(display = "Event receive error.")]
    EventReceiveError,
    #[fail(display = "Ready send failed.")]
    ReadySendFailed,
    #[fail(display = "Request timeout.")]
    RequestTimeout,
    #[fail(display = "Reached max backoff elapsed time.")]
    ReachedMaxBackoffElapsedTime,
    #[fail(display = "No client encoding.")]
    NoClientEncoding,
}

pub enum TokuErrorCode {
    // Normal is sent when the connection is closing cleanly.
    Normal = 0,
    // InvalidOp is sent when the connection receives an opcode it cannot handle.
    InvalidOpcode = 1,
    // UnsupportedVersion is sent when conn does not support a version.
    UnsupportedVersion = 2,
    // NoCommonEncoding is sent when there are no common encodings.
    NoCommonEncoding = 3,
    // InvalidEncoding is sent by the client if the server chooses an invalid encoding.
    InvalidEncoding = 4,
    // InvalidCompression is sent by the client if the server chooses an invalid compression.
    InvalidCompression = 5,
    // PingTimeout is sent when connection does not receive a pong within ping interval.
    PingTimeout = 6,
    // InternalServerError is sent when a single request dies due to an error.
    InternalServerError = 7,
}

impl TokuError {
    pub(crate) fn code(&self) -> TokuErrorCode {
        match self {
            TokuError::InvalidOpcode { .. } => TokuErrorCode::InvalidOpcode,
            TokuError::UnsupportedVersion { .. } => TokuErrorCode::UnsupportedVersion,
            TokuError::NoCommonEncoding { .. } => TokuErrorCode::NoCommonEncoding,
            TokuError::InvalidEncoding => TokuErrorCode::InvalidEncoding,
            TokuError::InvalidCompression => TokuErrorCode::InvalidCompression,
            TokuError::PingTimeout => TokuErrorCode::PingTimeout,
            // Normal close.
            TokuError::ConnectionCloseRequested => TokuErrorCode::Normal,
            _ => TokuErrorCode::InternalServerError,
        }
    }
}
