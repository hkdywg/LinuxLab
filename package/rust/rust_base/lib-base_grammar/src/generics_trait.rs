#![allow(unused_variables)]
#![allow(dead_code)]

use std::fmt::Display;

pub fn generics_trait_verify() {
    let integer = Point {x: 5, y : 10};
    let float = Point {x: 1.2, y: 3.14};
    println!("float x is {}", float.get_x());
    create_and_print::<i32>();

    let p1 = Point {x: 5, y: 2.1};
    let p2 = Point {x: "hello", y: 'c'};
    let p3 = p2.mixup(p1);
    println!("p3 x is {}, y is {}", p3.x, p3.y);

    let arr: [i32; 3] = [1, 2, 3];
    display_array(arr);
    let arr: [i32; 2] = [4, 5];
    display_array(arr);

    trait_verify();
}

fn create_and_print<T>() where T: From<i32> + Display {
    let a: T = 100.into();
    println!("a is: {}", a);
}


struct Point<T, U> {
    x: T,
    y: U,
}

impl<T, U> Point<T, U> {
    fn get_x(&self) -> &T {
        &self.x
    }
    fn mixup<V, W>(self, other: Point<V, W>) -> Point<T, W> {
        Point {
            x: self.x,
            y: other.y,
        }
    }
}

enum Ret<T, E> {
    Ok(T),
    Err(E),
}


fn display_array<T: std::fmt::Debug, const N: usize>(arr: [T; N]) {
    const MAX_TOTAL_SIZE: usize = 512;
    
    // 运行时检查：确保数组总大小不超过限制
    let total_size = core::mem::size_of::<T>() * N;
    if total_size > MAX_TOTAL_SIZE {
        panic!(
            "Array too large: {} bytes (max allowed: {} bytes). \
             Element size: {} bytes, array length: {}",
            total_size, MAX_TOTAL_SIZE,
            core::mem::size_of::<T>(), N
        );
    }
    
    println!("{:?}", arr);
}

const fn add(a: usize, b: usize) -> usize {
    a + b
}

const RESULT: usize = add(5, 10);

fn largest<T: PartialOrd + Copy>(list: &[T]) -> T {
    let mut largest = list[0];

    for &item in list.iter() {
        if item > largest {
            largest = item;
        }
    }

    largest
}

fn trait_verify() {
     let number_list = vec![12, 4, 55, 23, 99];
    
     let result = largest(&number_list);
     println!("the largest number is {}", result);

     let char_list = vec!['a', 'c', 'l', 'u'];
     let result = largest(&char_list);
     println!("the largest char is {}", result);

     let x = 3.14;
     let y = 9u8;

     draw1(Box::new(x));
     draw1(Box::new(y));
     draw2(&x);
     draw2(&y);
}

trait Draw {
    fn draw(&self) -> String;
}

impl Draw for u8 {
    fn draw(&self) -> String {
        format!("u8: {}", *self)
    }
}

impl Draw for f64 {
    fn draw(&self) -> String {
        format!("f64: {}", *self)
    }
}

fn draw1(x: Box<dyn Draw>) {
    println!("{}", x.draw());
}

fn draw2(x: &dyn Draw) {
    println!("{}", x.draw());
}
