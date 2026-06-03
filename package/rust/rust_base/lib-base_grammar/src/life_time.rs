#![allow(unused_variables)]
#![allow(dead_code)]

use std::fmt::Display;

pub fn life_time_verify() {
    let string_1 = String::from("abcd");
    let string_2 = "xyz";
    let string_3 = "debug";

    let result = longest(string_1.as_str(), string_2);
    println!("the longest string is {}", result);

    let len = longest_len(string_1.as_str(), string_2);
    println!("the longest len is {}", len);

    let ret = longest_with_display(string_1.as_str(), string_2, string_3);
}

fn longest<'a>(x: &'a str, y: &'a str) -> &'a str {
    if x.len() > y.len() {
        x
    } else {
        y
    }
}

fn longest_len(x: &str, y: &str) -> usize {
    if x.len() > y.len() {
        x.len()
    } else {
        y.len()
    }
}


fn first_word(s: &str) -> &str {
    let bytes = s.as_bytes();

    for (i, &item) in bytes.iter().enumerate() {
        if item == b' ' {
            return &s[0..i]
        }
    }

    &s[..]
}

fn longest_with_display<'a, T>(x: &'a str, y: &'a str,
            ann: T,) -> &'a str
where T: Display, {
    println!("Announcement! {}", ann);
    if x.len() > y.len() {
        x
    } else {
        y
    }
}
