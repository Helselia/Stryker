use crate::connection_handler::{ConnectionHandler, InternalEvent};
use crate::waiter::ResponseWaiter;
use crate::Config;
use failure::Error;
use futures::channel::mpsc::{channel, Sender};
use futures::channel::oneshot;
use futures::{SinkExt, StreamExt, TryFutureExt};
use toku_connection::{timeout_at, Connection, TokuError};
use std::net::SocketAddr;
use std::sync::atomic::{AtomicBool, Ordering::SeqCst};
use std::sync::Arc;
use std::sync::RwLock;
use std::time::Duration;
use tokio::task::spawn;
use tokio::time::Instant;

pub struct Client {
    connection: Connection<ConnectionHandler>,
    request_timeout: Duration,
    handshake_deadline: Instant,
    ready: Arc<AtomicBool>,
    ready_waiter_tx: Sender<oneshot::Sender<()>>,
    encoding: Arc<RwLock<Option<&'static str>>>,
}

const READY_CHAN_BUFFER_SIZE: usize = 100_000;

impl Client {
    pub async fn start_connect(address: SocketAddr, config: Config) -> Result<Client, Error> {
        let handshake_deadline = Instant::now() + config.handshake_timeout;
        let request_timeout = config.request_timeout;

        let handler = ConnectionHandler::new(config);

        let ready = Arc::new(AtomicBool::new(false));
        let (ready_tx, ready_rx) = oneshot::channel();
        let awaitable = timeout_at(
            handshake_deadline,
            ready_rx.map_err(|_canceled| Error::from(TokuError::ConnectionClosed)),
        );
        let (ready_waiter_tx, mut ready_waiter_rx) =
            channel::<oneshot::Sender<()>>(READY_CHAN_BUFFER_SIZE);
        let encoding = Arc::new(RwLock::new(None));

        let connection =
            Connection::spawn_from_address(address, handler, handshake_deadline, Some(ready_tx));

        let task_encoding = encoding.clone();
        let task_ready = ready.clone();
        spawn(async move {
            if let Ok(ready_encoding) = awaitable.await {
                *task_encoding.write().expect("Failed to write encoding") = Some(ready_encoding);
                task_ready.store(true, SeqCst);
                while let Some(tx) = ready_waiter_rx.next().await {
                    tx.send(()).ok();
                }
            }
        });

        Ok(Self {
            connection,
            handshake_deadline,
            request_timeout,
            ready,
            ready_waiter_tx,
            encoding,
        })
    }

    pub fn encoding(&self) -> Result<&'static str, Error> {
        if !self.is_ready() {
            return Err(TokuError::NotReady.into());
        }

        self.encoding
            .read()
            .expect("Failed to read encoding.")
            .clone()
            .ok_or_else(|| Error::from(TokuError::NoClientEncoding))
    }

    pub fn is_ready(&self) -> bool {
        self.ready.load(SeqCst)
    }

    /// Send a request to the server.
    pub async fn request(&self, payload: Vec<u8>) -> Result<Vec<u8>, Error> {
        if self.is_closed() {
            return Err(TokuError::ConnectionClosed.into());
        }
        if !self.is_ready() {
            return Err(TokuError::NotReady.into());
        }
        let (waiter, awaitable) = ResponseWaiter::new(self.request_timeout);
        let request = InternalEvent::Request { payload, waiter };
        self.connection.send(request)?;
        awaitable.await
    }

    /// Send a push to the server.
    pub async fn push(&self, payload: Vec<u8>) -> Result<(), Error> {
        if self.is_closed() {
            return Err(TokuError::ConnectionClosed.into());
        }
        if !self.is_ready() {
            return Err(TokuError::NotReady.into());
        }
        let push = InternalEvent::Push { payload };
        self.connection.send(push)
    }

    pub async fn await_ready(&self) -> Result<(), Error> {
        let (tx, rx) = oneshot::channel();

        let _result = timeout_at(
            self.handshake_deadline,
            self.ready_waiter_tx.clone().send(tx).map_err(Error::from),
        )
        .await?;
        timeout_at(
            self.handshake_deadline,
            rx.map_err(|_cancelled| Error::from(TokuError::ConnectionClosed)),
        )
        .await
    }

    pub fn is_closed(&self) -> bool {
        self.connection.is_closed()
    }
}
