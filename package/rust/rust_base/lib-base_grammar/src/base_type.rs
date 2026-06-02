pub fn base_bind() {
    let mut tmp_var = 5;
    println!("Hello, world! {}", tmp_var);
    tmp_var = 6;
    println!("Hello, world! {}", tmp_var);
    let _x = 0;

    let (a, mut b): (bool, bool) = (true, true);
    println!("a = {:?}, b = {:?}", a, b);
    b = false;
    println!("a = {:?}, b = {:?}", a, b);

    const MAX_POINTS: u32 = 100_000;
    println!("MAX_PINTS = {}", MAX_POINTS);

    {
        let b = true;
        println!("a = {:?}, b = {:?}", a, b);
    }
    println!("a = {:?}, b = {:?}", a, b);

    let spaces = "    ";
    let space_len = spaces.len();
    println!("spaces len = {}", space_len);
}

pub fn base_type() {
    let a: u8 = 255;
    let b = a.wrapping_add(20);
    println!("a = {}", b);

    let abc: (f32, f32, f32) = (0.1, 0.2, 0.3);
    let xyz: (f64, f64, f64) = (0.1, 0.2, 0.3);

    println!("abc (f32)");
    println!("  0.1 + 0.2: {:x}", (abc.0 + abc.1).to_bits());
    println!("        0.3: {:x}", (abc.2).to_bits());

    println!("xyz (f64)");
    println!("  0.1 + 0.2: {:x}", (xyz.0 + xyz.1).to_bits());
    println!("        0.3: {:x}", (xyz.2).to_bits());

    //assert!(abc.0 + abc.1 == abc.2);
    //assert!(xyz.0 + xyz.1 == xyz.2);
    
    //let value_a : i32 = 20;
    //let value_b : u32 = 20;
    //let addition = value_a + value_b;
    //println!("{} + {} = {}", value_a, value_b, addition);
    
    //for i in 0..5 {
    //    println!("{}", i);
    //}
    //for a in 'a'..'z' {
    //    println!("{}", a);
    //}

    let char_col = ['z', '阴'];
    println!("{} {}", char_col[0], char_col[1]);
    println!("字符'阴'占了{}字节的内存大小", std::mem::size_of_val(&char_col[1]));

    let f: bool = false;
    
    if f {
        println!("这段代码无意义, 不会打印出来");
    }


    println!("单元类型()占了{}字节的内存大小", std::mem::size_of_val(&()));

    let y = {
        let x = 3;
        x + 1
    };
    println!("y = {}", y);

    let z = if y % 2 == 1 { "odd" } else { "even" };
    println!("z is {}", z);
}

pub fn add_with_extra(x: i32, y: i32) -> i32 {
    let x = x + 1;
    let y = y + 1;
    x + y
}
