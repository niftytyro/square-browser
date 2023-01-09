use std::io::{self, Read, Write};
use std::net::TcpStream;

// TODO Add read/write timeouts
// TODO Kill connections after unresponsive for too long
// TODO Error handling
// TODO blocking vs non-blocking
// TODO handle ports and "localhost"

const MAX_BUFFER_LENGTH: usize = 1024;

fn main() {
    let url = read_url_input();

    let (scheme, host, path) = parse_url(&url);

    let mut socket = establish_connection(scheme, host);

    let response = fetch_page(&mut socket, path, host);

    println!("\n-------------------------------\n");
    println!("{}", response);
}

fn fetch_page(socket: &mut TcpStream, path: &str, host: &str) -> String {
    socket
        .write(format!("GET /{} HTTP/1.0\r\nHost: {}\r\n\r\n", path, host).as_bytes())
        .expect("Failed to write to socket.");
    socket
        .flush()
        .expect("Something wrong while flushing stream.");

    read_response(socket)
}

fn read_response(socket: &mut TcpStream) -> String {
    let mut buf = [0u8; MAX_BUFFER_LENGTH];

    let mut response_bytes: Vec<u8> = vec![];

    loop {
        let bytes_read = socket.read(&mut buf).expect("Failed to read stream");

        response_bytes.extend_from_slice(&buf[..bytes_read]);

        if bytes_read < MAX_BUFFER_LENGTH {
            break;
        }
    }

    let response = String::from_utf8(response_bytes).expect("Failed to convert to utf-8.");

    response
}

fn establish_connection(_: &str, host: &str) -> TcpStream {
    let socket = TcpStream::connect(format!("{}:{}", host, 80)).expect("Failed to connect.");
    socket
}

fn parse_url(url: &String) -> (&str, &str, &str) {
    let url_vec: Vec<&str> = url.splitn(2, "://").collect();
    let scheme = url_vec[0];
    let rest: Vec<&str> = url_vec[1].splitn(2, "/").collect();

    let host = rest[0];
    let path = if rest.len() > 1 { rest[1] } else { "" };

    (scheme, host, path)
}

fn read_url_input() -> String {
    let mut url = String::new();
    io::stdin().read_line(&mut url).expect("Failed to read");
    let url = String::from(url.trim());
    url
}
