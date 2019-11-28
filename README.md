# Gos
一个语法有那么点类似 C 的解释器，只是个瞎写的玩具。

由于本人太菜，变量命名、代码结构都很混乱 ~~，但是 it works~~ 。

（唯一的优点大概是可以比较简单地和 C++ 交互

examples 目录下的 example.gos 是个对目前实现功能的示例。

snake.gos 是一个简单的贪吃蛇程序。

它看起来是这样的：
```bash
#!/usr/bin/env gos
# This is a comment

# Sum
sum := 0;
for (i := 0; i <= 100; i += 1) {
    sum += i;
    # sum = sum + i;
}
println("The sum from 1 to 100 is: ", sum);

# Factorial
fact := func(x) {
    if (x == 0) {
        return(1);
    } else {
        return(x * fact(x - 1));
    }
    # return(if (x == 0) { 1 } else { x * fact(x - 1) });
}
println("10! = ", fact(10));

# Prime numbers
print("The prime numbers from 1 to 100 include: ");
isprime := func(x) {
    for (i := 2; i <= int(sqrt(float(x))); i += 1) {
        if (x % i == 0) {
            return(false);
        }
    }
    return(true);
}
for (i := 2; i <= 100; i += 1) {
    if (isprime(i)) {
        print(i, " ");
    }
}
println();

# Struct
struct (Box) {
    length = 0;
    breadth = 0;
    height = 0;
    read = func() {
        read(length, breadth, height);
    }
    getVolume = func() {
        return(length * breadth * height);
    }
}
x := Box;
x.read();
println(x.getVolume());
```
