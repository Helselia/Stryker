use bytesize::ByteSize;
use failure::Error;
use toku_bench_common::{configure_logging, make_socket_address};
use toku_server::{Config, RequestHandler, Server};
use std::future::Future;
use std::pin::Pin;
use std::time::Duration;

struct EchoHandler {}

impl RequestHandler for EchoHandler {
    fn handle_request(
        &self,
        request: Vec<u8>,
        _encoding: &'static str,
    ) -> Pin<Box<dyn Future<Output = Vec<u8>> + Send>> {
        Box::pin(async move { request })
    }

    fn handle_push(
        &self,
        _push: Vec<u8>,
        _encoding: &'static str,
    ) -> Pin<Box<dyn Future<Output = ()> + Send>> {
        Box::pin(async move {})
    }
}

#[tokio::main]
async fn main() -> Result<(), Error> {
    configure_logging()?;
    let config = Config {
        request_handler: EchoHandler {},
        max_payload_size: ByteSize::kb(5000),
        ping_interval: Duration::from_secs(5),
        handshake_timeout: Duration::from_secs(5),
        supported_encodings: &["msgpack", "identity"],
    };
    let server = Server::new(config);
    let result = server.listen_and_serve(make_socket_address()).await;
    println!("Run result={:?}", result);
    Ok(())
}
