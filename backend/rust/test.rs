fn p1(){
    let mut a:i32; 
    a=1;
    let mut a:i32;
    a=2;

}

fn p2(mut a: i32) -> i32{
	while a>0 {
        a=a-1;
    }
	return a;
}

fn main(){
    let mut num1 = 1;
    let mut num2 = 5;
	
    if(num1 < num2){
        p1();
    }else{
        let mut result = p2(num2);
    }

}