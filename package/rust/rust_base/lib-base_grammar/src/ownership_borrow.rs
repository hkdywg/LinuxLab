//!1.Rust中每一个值都被一个变量所拥有，该变量被称为值的所有者
//!2.一个值只能被一个变量所拥有，或者说一个值只能拥有一个所有者
//!3.当所有者(变量)离开作用域范围时，这个值将会丢弃(drop)

#[allow(unused_variables)]
#[allow(dead_code)]

pub fn ownership_verify() {
    let mut s1 = String::from("hello");
    s1.push_str(", world!");
    //let s2 = s1();    //这样会移交s1的所有权，后面的打印中会报错
    let s2 = s1.clone();    //深拷贝
    println!("s1 = {}, s2 = {}", s1, s2);

    //takes_ownership_string(s1);
    takes_ownership_str(&s1);   // 借用的方法可以使用，没有获得s1的所有权，函数结束时不会释放内存
    println!("s1 = {}, s2 = {}", s1, s2);

    let x = 5;
    make_copy(x);
    println!("x = {}", x);
}

fn takes_ownership_str(some_string: &str) {
    println!("{}", some_string);        
}

#[allow(dead_code)]
fn takes_ownership_string(some_string: String) {
    println!("{}", some_string);        
} // some_string移除作用域，并调用drop方法，占用的内存被释放

fn make_copy(some_integer: i32) {
    println!("{}", some_integer);
} // some_integer移除作用域，不会特殊操作


pub fn borrowing_verify() {
    let x = 10;
    let y = &x;

    println!("y = {}, *y = {}", y, *y);

    let mut s1 = String::from("hello");
    change_str(&mut s1);
    {
        let r1 = &mut s1;
        println!("r1 is {}", r1);
    }
    {
        let r2 = &mut s1;   //同一作用域内可变引用只能有一个, 如果去掉大括号则会报错
        println!("r2 is {}", r2);
    }

    {
        // 可变引用和不可变引用不能同时存在
        //let r3 = &s1;
        //let r4 = &mut s1;
        //println!("r3 = {}, r4 = {}", r3, r4);
    }
    let len = cal_string_len(&s1);
    println!("the length of \"{}\" is {}", s1, len);
}

fn cal_string_len(s: &String) -> usize {
    s.len()
}

fn change_str(some_string: &mut String) {
    some_string.push_str(", world");
}
