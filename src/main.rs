use std::io::{self, Read, Write};
use std::net::{SocketAddr, TcpStream};

// TODO Add read/write timeouts
// TODO Kill connections after unresponsive for too long
// TODO Error handling
// TODO blocking vs non-blocking

fn main() {
    let url = recieve_url_input();

    let (host, path) = parse_url(&url);

    println!("Host: {}\nPath: {}", host, path);
}

fn parse_url(url: &String) -> (&str, &str) {
    let url_vec: Vec<&str> = url.splitn(2, "://").collect();
    let scheme = url_vec[0];
    let rest: Vec<&str> = url_vec[1].splitn(2, "/").collect();

    let host = rest[0];
    let path = if rest.len() > 1 { rest[1] } else { "/" };

    (host, path)
}

fn recieve_url_input() -> String {
    let mut url = String::new();
    io::stdin().read_line(&mut url).expect("Failed to read");
    let url = String::from(url.trim());
    url
}
