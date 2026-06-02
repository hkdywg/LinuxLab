#![allow(unused_variables)]
#![allow(dead_code)]

enum Action {
    Say(String),
    MoveTo(i32, i32),
    ChangeColorRGB(u16, u16, u16),
}

pub fn option_match_verify() {
    match_verify();
}

fn match_verify() {
    let actions = [
        Action::Say("Hello Rust".to_string()),
        Action::MoveTo(1, 2),
        Action::ChangeColorRGB(255, 255, 0),
    ];

    for action in actions {
        match action {
            Action::Say(s) => {
                println!("{}", s);
            },
            Action::MoveTo(x,y) => {
                println!("point from (0,0) to ({}, {})", x, y);
            },
            Action::ChangeColorRGB(r, g, _) => {
                println!("change color into '(r:{}, g:{}, b:0)', 'b' has been ignore",
                r, g);
            }
        }
    }

    let v = 3;
    match v {
        3 => println!("three"),
        _ => (),
    }

    let foo = 'f';
    assert!(matches!(foo,  'A'..='Z' | 'a'..='z'));

    let five = Some(5);
    let six = plus_one(five);
    let none = plus_one(None);
    println!("six = {:?}", six);

    let x = 2;
    match x {
        1..=3 => println!("one througn three"),
        4 | 5 => println!("four or five"),
        6 => println!("six"),
        _ => println!("something else"),
    }
}


fn plus_one(x: Option<i32>) -> Option<i32> {
    match x {
        None => None,
        Some(i) => Some(i + 1),
    }
}
