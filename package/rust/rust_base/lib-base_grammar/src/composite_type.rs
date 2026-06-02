#![allow(unused_variables)]
#![allow(dead_code)]

pub fn composite_type_verify() {
    //slice_verify();
    //tuple_verify();
    //struct_verify();
    enum_verify();
}

pub fn slice_verify() {
    let mut s = String::from("hello, world!");

    let len = s.len();
    s.insert(len, '!');

    let mut replace = s.replace("hello", "Hello");
    replace.pop();
    println!("replace is {}", replace);

    let s1 = &s[0..6];
    let s2 = &s[6..];
    let s3 = &s[..];
    println!("s1 = {} s2 = {} s3 = {}", s1, s2, s3);
    println!("{} first word is {}", s, first_word(&s));

    let a = [1, 2, 3, 4, 5];
    let slice = &a[1..3];
    println!("a = {:?}", slice[0]);
}

fn first_word(s: &String) -> &str {
    &s[..1]
}

pub fn tuple_verify() {
    let tup: (i32, f64, u8) = (10, 3.14, 128);
    let (x, y, z) = tup;
    println!("x = {} y = {} z = {}", x, y, z);
    println!("tup[1] = {}", tup.1);

    let s = String::from("hello");
    let (s1, len) = cal_string_len(s);
    println!("the length of {} is {}", s1, len);
}

fn cal_string_len(s: String) -> (String, usize) {
    let len = s.len();

    (s, len)
}

struct User {
    active: bool,
    user_name: String,
    email: String,
    sign_in_count: u64,
}

pub fn struct_verify() {
    let mut user_1: User = User {
        email: String::from("example@gmail.com"),
        user_name: String::from("kevin"),
        active: true,
        sign_in_count: 1,
    };
    user_1.active = false;
    println!("user {} email is {}", user_1.user_name, user_1.email);

    let user_2 = build_user(String::from("simple@gmail.com"), String::from("kaka"));

    let user_3 = User {
        user_name: String::from("stevn"),
        ..user_1
    };
    println!("user {} email is {}", user_3.user_name, user_3.email);
    //下面打印会出错，因为user_1的除了user_name的所有权全部移交到user_3，使用user_1.email时会出错
    //但如果只打印user_1.user_name则不会有问题
    //println!("user {} email is {}", user_1.user_name, user_1.email);
    

    struct Color(i32, i32, i32);
    let black = Color(0, 0, 0);
}

fn build_user(email: String, user_name: String) -> User {
    User {
        email: email,
        user_name: user_name,
        active: true,
        sign_in_count: 1,
    }
}

enum PokerSuit {
    Clubs,
    Spades,
    Diamonds,
    Hearts,
}

fn enum_verify() {
    let heart = PokerSuit::Hearts;
    let diamond = PokerSuit::Diamonds;

    print_suit(heart);
    print_suit(diamond);
}

fn print_suit(card: PokerSuit) {
    //println!("{}", card);
}

fn array_verify() {
    let a = [1, 2, 3, 4, 5];
    let b: [i32; 5] = [1, 2, 3, 4, 5];
    let c = [3; 5];

    let first = a[0];
    let slice: &[i32] = &a[1..3];
}

