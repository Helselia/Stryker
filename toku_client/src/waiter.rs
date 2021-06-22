use failure::Error;
use futures::channel::oneshot::{self, Sender};
use futures::TryFutureExt;
use toku_connection::{timeout_at, TokuError};
use std::future::Future;
use std::time::Duration;
use tokio::time::Instant;

#[derive(Debug)]
pub struct ResponseWaiter {
    tx: Sender<Result<Vec<u8>, Error>>,
    pub deadline: Instant,
}

impl ResponseWaiter {
    /// Creates a new response waiter that will wait until the specified timeout.
    /// The returned future will resolve when someone calls waiter.notify().
    ///
    /// # Arguments
    ///
    /// * `timeout` - the `Duration` of time to wait before giving up on the request
    ///
    /// # Errors
    ///
    /// `TokuError::RequestTimeout` or some other error from the server.
    ///
    pub fn new(timeout: Duration) -> (Self, impl Future<Output = Result<Vec<u8>, Error>>) {
        let (tx, rx) = oneshot::channel();

        let deadline = Instant::now() + timeout;

        let awaitable = async move {
            let rx = rx.map_err(|_cancelled| Error::from(TokuError::ConnectionClosed));
            timeout_at(deadline, rx)
                .await
                .unwrap_or_else(|_e| Err(TokuError::RequestTimeout.into()))
        };

        (Self { tx, deadline }, awaitable)
    }

    /// Notify the waiter that a result was received.
    pub fn notify(self, result: Result<Vec<u8>, Error>) {
        if let Err(_e) = self.tx.send(result) {
            if self.deadline > Instant::now() {
                warn!("Waiter is no longer listening.")
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use tokio::runtime::Runtime;
    use tokio::task::spawn;
    use tokio::time::delay_for;

    #[test]
    fn it_receives_ok() {
        let (waiter, awaitable) = ResponseWaiter::new(Duration::from_secs(5));
        let result = Runtime::new().unwrap().block_on(async {
            spawn(async {
                waiter.notify(Ok(vec![]));
            });
            awaitable.await
        });
        assert!(result.is_ok())
    }

    #[test]
    fn it_receives_error() {
        let (waiter, awaitable) = ResponseWaiter::new(Duration::from_secs(5));

        let result: Result<Vec<u8>, Error> = Runtime::new().unwrap().block_on(async {
            spawn(async {
                waiter.notify(Err(TokuError::ConnectionClosed.into()));
            });
            awaitable.await
        });
        assert!(result.is_err())
    }

    #[test]
    fn it_times_out() {
        let (waiter, awaitable) = ResponseWaiter::new(Duration::from_millis(1));

        let result = Runtime::new().unwrap().block_on(async {
            spawn(async {
                delay_for(Duration::from_millis(50)).await;
                waiter.notify(Ok(vec![]));
            });
            awaitable.await
        });
        assert!(result.is_err())
    }
}
