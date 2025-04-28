### std::memset
`std::memset` 是 C/C++ 标准库中的一个函数，位于 `<cstring>` 头文件中，用于将一块内存的每个字节设置为指定的值。它通常用于快速初始化或清空内存块（如数组、结构体等）。

---

**函数原型**
```cpp
#include <cstring>  // 需要包含的头文件

void* memset(void* ptr, int value, size_t num);
```
- **参数**：
  - `ptr`：指向要填充的内存块的指针（起始地址）。
  - `value`：要设置的值（以 `int` 形式传递，但实际按字节填充）。
  - `num`：要填充的字节数。
- **返回值**：返回 `ptr` 的原始指针。

---

1. 初始化数组为 0
```cpp
int arr[10];
std::memset(arr, 0, sizeof(arr));  // 将 arr 所有字节设为 0
```

2. 设置内存为特定值
```cpp
char buffer[100];
std::memset(buffer, 'A', sizeof(buffer));  // 每个字节被设为 'A' 的 ASCII 值（65）
```

3. 清空结构体
```cpp
struct MyStruct { int a; float b; };
MyStruct s;
std::memset(&s, 0, sizeof(s));  // 将 s 的所有成员置零
```

### constexpr
**1. `constexpr` 变量**

用于定义编译时常量（必须能在编译时确定值）：
```cpp
constexpr int size = 10;           // 编译时常量
constexpr double pi = 3.1415926;   // 编译时计算的常量
```
- 和 `const` 的区别：`const` 只保证运行时不修改，而 `constexpr` 要求值必须在编译时确定。
- 用途：定义数组大小、模板参数等需要编译时常量的场景。

---

**2. `constexpr` 函数**

函数可以在编译时求值（如果参数是编译时常量）：
```cpp
constexpr int factorial(int n) {
    return (n <= 1) ? 1 : n * factorial(n - 1);
}

constexpr int fact_5 = factorial(5);  // 编译时计算 120
```
- 如果传入运行时变量（如 `int n; factorial(n);`），则函数在运行时计算。
- C++14 放宽了限制（允许局部变量、循环等）。

---

**3. `constexpr` 和对象**

可以修饰构造函数，生成编译时常量对象：
```cpp
struct Point {
    constexpr Point(int x, int y) : x(x), y(y) {}
    int x, y;
};

constexpr Point p(10, 20);  // 编译时创建的常量对象
```


### 移动与复制构造
**复制构造**

创建对象时，通过深拷贝（Deep Copy）生成一个与原对象完全独立的副本
```cpp
class MyArray {
    int* data;
    size_t size;
public:
    // 复制构造函数（深拷贝）
    MyArray(const MyArray& other) : size(other.size) {
        data = new int[size];
        std::copy(other.data, other.data + size, data);
    }
};
```
**移动构造**

通过“窃取”临时对象（右值）的资源来初始化新对象，避免不必要的深拷贝，提升性能

直接接管原对象的资源（如指针），并将原对象的资源置空，避免析构时重复释放
```cpp
ClassName(ClassName&& other) noexcept;
{
    handle = other.handle; 
    other.handle = VK_NULL_HANDLE;
}
```

### 操作符重载
**成员函数**
```cpp
class MyArray {
    int* data;
    size_t size;
public:
    // 赋值运算符（深拷贝）
    MyArray& operator=(const MyArray& other) {
        if (this != &other) {  // 防止自赋值
            delete[] data;      // 释放原有资源
            size = other.size;
            data = new int[size];
            std::copy(other.data, other.data + size, data);
        }
        return *this;  // 支持链式赋值（a = b = c）
    }
};
```
**非成员函数**
```cpp
class MyArray {
    // ...
public:
    friend MyArray operator+(const MyArray& a, const MyArray& b);
};

MyArray operator+(const MyArray& a, const MyArray& b) {
    MyArray result(a.size + b.size);
    std::copy(a.data, a.data + a.size, result.data);
    std::copy(b.data, b.data + b.size, result.data + a.size);
    return result;
}
```


### 悬空引用
如果将一个临时变量的引用传递给类的构造函数，并用它来初始化类的私有成员后，该成员仍然引用了这个临时变量。而临时变量在离开作用域后就会被销毁，导致类的成员引用变成了悬空引用（dangling reference）。当类实例尝试访问这个成员时，就可能出现未定义行为，导致程序崩溃或出现意外的结果

传入参数时依旧注意引用以防止不必要的复制，但不要将类的成员设置为引用：
```cpp
// std::string m_FilePath;
// 而不是std::string& m_FilePath;

Shader::Shader(const std::string& filepath): m_FilePath(filepath), m_Program(0);
```

### 为编辑器添加提示信息
```
/**
 * @brief 获取布局中的所有元素。
 * @return 一个常量引用，包含布局中的所有 BufferLayoutElement 元素。
 */
```
@brief：简短描述函数的功能。
@param：描述参数的名称和功能。
@tparam：说明模板参数的用途。
@return：描述返回值（如果有）

### 在类A中管理类B
1. 直接定义类B的实例（成员变量）
注意这样类B会在A之前构造，如果类B的构造函数有参数，那么类A的构造函数必须包含此参数，且通过成员初始化列表初始化B
2. 使用指针管理类B
```cpp
class B {
public:
    B() { std::cout << "B constructed\n"; }
    ~B() { std::cout << "B destructed\n"; }
};

class A {
private:
    B* b;  // 使用指针管理类B
public:
    A() { b = new B(); std::cout << "A constructed\n"; }
    ~A() { delete b; std::cout << "A destructed\n"; }
};

int main() {
    A a;  // A会创建并管理B
    return 0;
}
```
3. 使用智能指针（推荐）
为了避免手动管理内存和潜在的内存泄漏问题，可以使用 std::unique_ptr 或 std::shared_ptr 来管理类B的实例。std::unique_ptr 表示类B的唯一所有权，而 std::shared_ptr 则允许多个所有者
```cpp
#include <memory>

class B {
public:
    B() { std::cout << "B constructed\n"; }
    ~B() { std::cout << "B destructed\n"; }
};

class A {
private:
    std::unique_ptr<B> b;  // 使用unique_ptr管理类B
public:
    A() : b(std::make_unique<B>()) { std::cout << "A constructed\n"; }
    ~A() { std::cout << "A destructed\n"; }
};

int main() {
    A a;  // A会创建并自动管理B
    return 0;
}
```

### 函数返回多个值

### 模板类的自动构造与完美转发（Perfect Forwarding）
```cpp
#include <iostream>
#include <vector>
#include <string>

template <typename T>
class MyClass {
private:
    T value;

public:
    // 模板构造函数，通过传入的值自动推导类型
    template <typename U>
    MyClass(U&& val) : value(std::forward<U>(val)) {
        std::cout << "Constructor called with value: " << value << std::endl;
    }

    void display() {
        std::cout << "Stored value: " << value << std::endl;
    }
};

int main() {
    // 通过传递不同类型的值来创建 MyClass 对象，构造函数自动推导类型
    MyClass obj1(10);         // 类型为 int
    obj1.display();

    MyClass obj2(3.14);       // 类型为 double
    obj2.display();

    MyClass obj3("Hello");    // 类型为 const char*
    obj3.display();

    MyClass obj4(std::vector<int>{1, 2, 3}); // 类型为 std::vector<int>
    obj4.display();

    return 0;
}

```
### 数组作为参数
如果使用原生数组，并且数组大小不是编译时已知，那传递数组大小参数是不可避免的
```cpp
void myFunction(int* arr, int size) {
    for (int i = 0; i < size; i++) {
        // 处理数组元素
    }
}
```
可以使用 std::vector，可以在不传递大小参数的情况下直接获取数组大小
```cpp
#include <vector>

void myFunction(std::vector<int>& arr) {
    for (int i = 0; i < arr.size(); i++) {
        // 处理数组元素
    }
}
```
模板函数： 可以使用模板函数来让编译器推导数组的大小信息：
```cpp
template <size_t N>
void myFunction(int (&arr)[N]) {
    for (size_t i = 0; i < N; i++) {
        // 处理数组元素
    }
}
```

使用模板vector
```cpp
#include <vector>
#include <iostream>

template <typename T>
void myFunction(const std::vector<T>& vec) {
    for (const T& element : vec) {
        std::cout << element << " ";
    }
    std::cout << std::endl;
}
```

### new
```c++
int size;
std::cin >> size;
int* arr = new int[size]; // 创建动态长度的数组

// 使用数组
// ...

delete[] arr; // 释放内存
```
### 指针的指针
#### 修改指针本身

如果你需要在函数内部改变一个指针的指向（例如，重新分配内存或改变指针所指向的对象），可以使用双指针。例如：

```c++
cppCopy codevoid changePointer(int** p) {
    *p = new int(20); // 修改指针p的指向
}

int main() {
    int* ptr = nullptr;
    changePointer(&ptr); // ptr现在指向新分配的内存
    delete ptr; // 不要忘记释放内存
    return 0;
}
```

#### 作为函数返回多个值的方式

使用双指针也可以作为函数返回多个值的一种方式，尤其是在需要返回状态或结果的情况下：

```c++
cppCopy codevoid getValues(int** a, int** b) {
    *a = new int(5);
    *b = new int(10);
}

int main() {
    int* x = nullptr;
    int* y = nullptr;
    getValues(&x, &y); // x和y被赋值

    // 使用x和y后要释放内存
    delete x;
    delete y;

    return 0;
}
```

#### 动态内存分配

双指针常用于动态创建二维数组或更高维度的数据结构。例如：

```C++
cppCopy codevoid createArray(int*** arr, int rows, int cols) {
    *arr = new int*[rows];         // 分配行
    for (int i = 0; i < rows; ++i) {
        (*arr)[i] = new int[cols]; // 分配每行的列
    }
}

int main() {
    int** array = nullptr;
    createArray(&array, 3, 4); // 创建一个3x4的二维数组

    // 使用完后要释放内存
    for (int i = 0; i < 3; ++i) {
        delete[] array[i];
    }
    delete[] array;

    return 0;
}
```

#### 接收可变长度字符串
```c++
// 这里的const GLchar *const *string是一个双指针
void glShaderSource(GLuint shader, GLsizei count, const GLchar *const *string, const GLint *length)

// shaderSource是一个数组，其中每一个元素都是一个指向字符的指针
const char* shaderSource[] = {
    "#version 330 core\n",
    "void main() {\n",
    "    // Shader code here\n",
    "}\n"
};
// Hint: C++的字符串末尾自动添加'\0'以标示结尾

// 调用 glShaderSource
glShaderSource(shader, 4, shaderSource, nullptr);
```

### typedef

```c++
typedef existing_type new_type_name;

typedef unsigned long ulong;
ulong a = 100;  // 使用别名 ulong 代替 unsigned long

struct Person {
    char name[50];
    int age;
};
typedef struct Person Person;  // 创建结构体的别名
Person p;  // 使用别名创建结构体实例

//函数指针
typedef void (*FuncPtr)(int, double);  // 定义函数指针类型
void myFunction(int a, double b) {
    // 函数实现
}
FuncPtr f = myFunction;  // 使用类型别名

//数组
typedef int IntArray[10];  // 定义一个整型数组的别名
IntArray arr;  // 创建一个包含 10 个整数的数组
```

关于typedef在函数指针的用法：
```c++
#include <iostream>

typedef void (*FuncPtr)(int, double);  // 定义一个函数指针类型

// 定义几个符合签名的函数
void functionA(int a, double b) {
    std::cout << "Function A: " << a << ", " << b << std::endl;
}

void functionB(int a, double b) {
    std::cout << "Function B: " << a * 2 << ", " << b * 2 << std::endl;
}

void functionC(int a, double b) {
    std::cout << "Function C: " << a + 10 << ", " << b + 10 << std::endl;
}

// 主函数
int main() {
    FuncPtr funcArray[] = {functionA, functionB, functionC};  // 使用 typedef 定义的函数指针数组

    for (int i = 0; i < 3; ++i) {
        funcArray[i](5, 3.5);  // 通过函数指针调用每个函数
    }

    return 0;
}
```

### vector

`vector<type> name(10)`新建长度为10的数组 并将所有元素置0

`vector<type> name[10]`新建10个数组 使用`name[index][i]`访问对应元素 或`name[index].begin()`等函数

**erase **对于vector 使用`vec.erase(vec.begin()+i)`删除某一元素

`vector <vector<int>> vec` 创建动态二维数组

`vector.back()` `vector.front()` 获取数组最后(最前)一个元素的引用 不同于 `.begin() .end()` 返回一个迭代器

### quene

1. push() 在队尾插入一个元素
2. pop() 删除队列第一个元素
3. size() 返回队列中元素个数
4. empty() 如果队列空则返回true
5. front() 返回队列中的第一个元素
6. back() 返回队列中最后一个元素

### void*

**通用性**：`void*` 指针可以指向任何类型的数据，使得函数参数或返回值可以是任意类型的指针。这对于编写泛型函数或数据结构（如链表、堆栈等）非常有用。

**类型不确定性**：在一些场合下，数据类型可能在编译时并不确定，使用 `void*` 可以暂时绕过这种限制，直到实际使用时再进行类型转换

**类型转换**：`void*` 指针在使用前必须被显式转换为目标类型的指针。转换时要确保类型匹配，否则可能导致未定义行为

```c
int a = 10;
void* ptr = &a;
int* int_ptr = (int*)ptr;
```

**用`void*` 实现简单的通用编程(常用于c，c++尽量使用模板):**

使用 `void*` 的通用链表节点（C 语言）

```c
#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    void* data;
    struct Node* next;
} Node;

Node* createNode(void* data) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->data = data;
    newNode->next = NULL;
    return newNode;
}

void printIntNode(Node* node) {
    int* data = (int*)node->data;
    printf("%d\n", *data);
}

int main() {
    int value = 42;
    Node* node = createNode(&value);
    printIntNode(node);
    free(node);
    return 0;
}
```

使用模板的通用链表节点（C++ 语言）

```c++
#include <iostream>

template <typename T>
struct Node {
    T data;
    Node* next;
    Node(T data) : data(data), next(nullptr) {}
};

template <typename T>
void printNode(Node<T>* node) {
    std::cout << node->data << std::endl;
}

int main() {
    Node<int> node(42);
    printNode(&node);
    return 0;
}
```

### 模板特化(Template Specialization)

函数模板的特化：当函数模板需要对某些类型进行特别处理，称为函数模板的特化。例如：
```cpp
​     template <class T>
1    bool IsEqual(T t1, T t2)
2    {
3         return t1 == t2;
4    }
5
6    int main()
7    {
8         char str1[] = "Hello";
9         char str2[] = "Hello";
10        cout << IsEqual(1, 1) << endl;
11        cout << IsEqual(str1, str2) << endl;  //输出0
12        return 0;
13   }
```
代码11行比较字符串是否相等。由于对于传入的参数是char *类型的，max函数模板只是简单的比较了传入参数的值，即两个指针是否相等，因此这里打印0。显然，这与我们的初衷不符。因此，max函数模板需要对char *类型进行特别处理，即特化：
```cpp
1     template <>
2    bool IsEqual(char* t1, char* t2)      //函数模板特化
3    {
4         return strcmp(t1, t2) == 0;
5    }
```
可以将模板类型作为一个参数：



### extern关键字

#### 1. **外部变量的声明**

当变量在一个文件中定义，但在其他文件中使用时，可以使用`extern`关键字在其他文件中声明该变量。例如：

**文件1 (`file1.c`)：**

```c++
int globalVar = 42;  // 定义变量
```

**文件2 (`file2.c`)：**

```c++
extern int globalVar;  // 声明变量
void someFunction() {
    printf("%d\n", globalVar);  // 使用变量
}
```

#### 2. **链接多个文件**

在大型程序中，`extern`常用于链接多个文件。例如，一个变量在一个文件中定义，在多个其他文件中使用：

**头文件 (`globals.h`)：**

```c++
extern int sharedVar;  // 声明变量
```

**源文件1 (`file1.c`)：**

```c++
#include "globals.h"
int sharedVar = 100;  // 定义变量
```

**源文件2 (`file2.c`)：**

```c++
#include "globals.h"
void someOtherFunction() {
    printf("%d\n", sharedVar);  // 使用变量
}
```

### 类的指针

![image-20220928021414620](C:\Users\CwQ\Documents\笔记\图片\image-20220928021414620.png)

基类指针可以指向派生类对象 但仅访问基类的成员 不能访问派生类中的新成员

基类指针仅访问基类成员和函数 派生类指针可以访问基类和新定义的成员和函数 一般将基类指针转换成派生类指针是不安全的

### static_cast

明确隐式类型转换

static_cast<目标类型>(原数据)

**可以用于低风险的转换**

- 整型和浮点型
- 字符与整形
- 转换运算符
- 空指针转换为任何目标类型的指针

**不可以用与风险较高的转换**

- 不同类型的指针之间互相转换
- 整型和指针之间的互相转换
- 不同类型的引用之间的转换

### 枚举类型enum的命名空间 作用域

在类中使用 类名作为命名空间

class A {

public:

​	enum{ NUM }

}

A::NUM;

### count 与 count_if

```c++
struct student
{
    string name;
    int score;
};
bool compare(student a)
{
    return 90<a.score;
}
int main()
{
    int n;
    cin>>n;
    vector<student> V;
    for(int i=0;i<n;i++)
    {
        student temp;
        cin>>temp.name>>temp.score;
        V.push_back(temp);
    }
    cout<<count_if(V.begin(),V.end(),compare)<<endl;
    return 0;
}
```



### static

1. 静态局部变量：
在函数内部定义的静态变量会在第一次执行到该变量声明时初始化，并且在整个程序运行期间都存在。这意味着它的生命周期超过了函数调用的范围，但是作用域仍然限制在声明它的函数内
2. 静态全局变量/函数：
当在文件作用域（即不在任何函数或类内部）声明一个变量或函数为static时，这会限制其作用域仅限于声明它的文件。也就是说，其他源文件中的代码无法通过外部链接访问到这个变量或函数
3. 静态类成员：
类中的静态成员属于整个类而非类的某个特定对象。所有对象共享同一个静态成员的实例。静态数据成员必须在类定义之外初始化，而静态成员函数则可以直接通过类名调用，无需创建类的对象





### 枚举类
建议都使用枚举类，有较高的类型安全和作用域控制
```c++
enum class Color {
    Red,
    Green,
    Blue
};

int main() {
    Color myColor = Color::Green;  // 需要使用 Color:: 前缀
    int colorValue = static_cast<int>(myColor);  // 显式转换为整数
    return 0;
}
```
也可以显示的指定类型的值：
```c++
enum class Color {
    Red = 1,
    Green = 2,
    Blue = 3
};
```

普通枚举类名不能作为命名空间

### string



### C++与C混编

**解决C不支持函数重载**

(1)函数重载机制是C++的特性，而C语言并没有，根据向前兼容原则，需要C++去兼容C，而不能要求C语言去支持函数重载机制；
(2)解决方法就是C++在需要和C对接的局部不采用函数重载机制，向C兼容；
(3)在C++中，用extern “C”{}括起来的内容表示向C兼容，不要使用函数重载机制；
注意点：extern “C”{}是C++中支持的，在C中是没有extern “C”{}这个用法的，C语言使用编译会报错；

```c++
#ifdef __cplusplus
extern "C"{
#endif

	······

#ifdef __cplusplus
}
#endif
```

### explicit

`explicit`关键字在C++中用于构造函数或转换运算符，主要目的是防止隐式转换。它告诉编译器该构造函数或转换运算符只能通过直接调用来使用，而不能通过隐式类型转换来使用。

考虑以下类定义：

```c++
class Foo {
public:
    Foo(int x) { /* ... */ }
};
```

使用上述类时，可以进行隐式转换：

```c++
void doSomething(Foo f) { /* ... */ }

doSomething(42);  // 隐式转换：int 42 -> Foo(42)
```

为了防止这种隐式转换，可以使用`explicit`关键字：

```c++
class Foo {
public:
    explicit Foo(int x) { /* ... */ }
};
```

现在，隐式转换将被禁止，必须显式调用构造函数：

```c++
doSomething(Foo(42));  // 必须显式调用
```

### 指针函数

```c++
// 返回int指针的函数，下面写法皆可
int *fun(int x,int y);
int * fun(int x,int y)；
int* fun(int x,int y);
```

### 函数指针

```c++
#include <iostream>

int add(int a, int b) {
    return a + b;
}

int subtract(int a, int b) {
    return a - b;
}

int main() {
    // 声明函数指针
    int (*funcPtr)(int, int);

    // 将函数地址赋值给函数指针
    funcPtr = add;
    std::cout << "Add: " << funcPtr(2, 3) << std::endl; // 输出：Add: 5

    // 将其他函数地址赋值给函数指针
    funcPtr = subtract;
    std::cout << "Subtract: " << funcPtr(5, 3) << std::endl; // 输出：Subtract: 2

    return 0;
}
```

### const

**1. 常量变量**

声明一个常量变量，其值不能被修改：

```c++
const int x = 10;
// x = 20; // 错误：x是常量，不能被修改
```

**2. 常量指针和指向常量的指针**

- 指向常量的指针：指针指向的值不能通过该指针修改，但指针本身可以改变。

  ```c++
  const int a = 10;
  const int b = 20;
  const int *ptr = &a; // ptr指向a
  // *ptr = 20; // 错误：不能修改ptr指向的值
  ptr = &b; // 合法：可以改变ptr的指向
  ```

- 常量指针：指针本身不能改变，但可以通过该指针修改指向的值。

  ```c++
  int a = 10;
  int b = 20;
  int *const ptr = &a; // 常量指针，ptr不能改变
  *ptr = 20; // 合法：可以修改ptr指向的值
  // ptr = &b; // 错误：不能改变ptr的指向
  ```

- 指向常量的常量指针：指针本身和指向的值都不能修改。

  ```c++
  const int a = 10;
  const int *const ptr = &a; // 指向常量的常量指针
  // *ptr = 20; // 错误：不能修改ptr指向的值
  // ptr = &b; // 错误：不能改变ptr的指向
  ```

**3. 常量引用**

常量引用用于避免在传递大型对象时进行拷贝，并防止修改引用的对象：

```c++
void printValue(const int& value) {
    // value不能被修改
    std::cout << value << std::endl;
}

int main() {
    int x = 10;
    printValue(x); // x不会被修改
    return 0;
}
```

**4. 常量成员函数**

在类中，常量成员函数不能修改对象的成员变量。常量成员函数的声明在函数名之后加上 `const` 关键字：

```c++
class MyClass {
public:
    int getValue() const {
        return value;
    }

private:
    int value = 10;
};

int main() {
    MyClass obj;
    std::cout << obj.getValue() << std::endl; // 合法：getValue是常量成员函数
    return 0;
}
```

**5. 函数参数和返回值的常量性**

- 常量函数参数：防止在函数内部修改参数。

  ```c++
  void printValue(const int value) {
      // value不能被修改
      std::cout << value << std::endl;
  }
  ```

- 常量返回值：防止对返回值的修改。

  ```c++
  const int getValue() {
      return 10;
  }
  ```

**6. 常量类成员**

类成员可以是常量，必须在构造函数的初始化列表中初始化：

```c++
class MyClass {
public:
    MyClass(int v) : value(v) {}
    int getValue() const {
        return value;
    }

private:
    const int value; // 常量成员
};

int main() {
    MyClass obj(10);
    std::cout << obj.getValue() << std::endl; // 输出：10
    return 0;
}
```

### static和friend创建类的工具函数

- 使用friend，可访问私有成员

  ```c++
  class MyVector {
  public:
      //成员变量
      double x, y, z;
  	
      // 非成员函数
      friend double Dist(const MyVector&, const MyVector&);
  };
  
  double Dist(const MyVector& v1, const MyVector& v2) {
      return sqrt(pow(v1.x - v2.x, 2) + pow(v1.y - v2.y, 2) + pow(v1.z - v2.z, 2));
  }
  ```

- 如果你希望在类内部实现一些工具性的功能，并且这些功能不需要访问对象的具体状态，那么可以考虑将它们声明为`static`成员函数

  ```c++
  class MyVector {
  public:
      // 成员变量
      double x, y, z;
  
      // 静态成员函数声明
      static double Dist(const MyVector& v1, const MyVector& v2);
  };
  
  // 静态成员函数定义
  double MyVector::Dist(const MyVector& v1, const MyVector& v2) {
      return sqrt(pow(v1.x - v2.x, 2) + pow(v1.y - v2.y, 2) + pow(v1.z - v2.z, 2));
  }
  
  // 在其他地方可以直接调用
  double distance = MyVector::Dist(vec1, vec2);
  ```

两种方式定义的函数都是非成员函数，可以在不定义实例的情况下直接使用

## Error

头文件中类成员函数的定义只能放在类体内 不能在类体外定义(多重定义的符号)

与库函数或变量重名(expected unqualified-id)

## 使用 std::shared_ptr 进行全局对象管理


## 智能指针
### **`std::shared_ptr` 的典型用法**
#### **1. 共享所有权的资源管理**
适用于 **多个对象共享一个资源**，如 **插件系统、GUI 组件、树结构等**。

```cpp
#include <iostream>
#include <memory>

struct Resource {
    Resource(int id) : id(id) { std::cout << "Resource " << id << " acquired\n"; }
    ~Resource() { std::cout << "Resource " << id << " released\n"; }
    int id;
};

void useResource(std::shared_ptr<Resource> res) {
    std::cout << "Using resource " << res->id << " with reference count: " << res.use_count() << "\n";
}

int main() {
    std::shared_ptr<Resource> res1 = std::make_shared<Resource>(1);
    {
        std::shared_ptr<Resource> res2 = res1; // 引用计数+1
        useResource(res2); // 共享资源
    } // res2 作用域结束，引用计数-1

    std::cout << "Reference count after res2 is destroyed: " << res1.use_count() << "\n";
} // res1 作用域结束，引用计数变为0，资源释放
```
**输出**
```
Resource 1 acquired
Using resource 1 with reference count: 2
Reference count after res2 is destroyed: 1
Resource 1 released
```
**适用场景**：
- 共享一个对象（如数据库连接、日志管理器等）。
- 适用于 **动态创建** 的对象，避免手动 `delete`。

---

#### **2. 解决 `shared_ptr` 的循环引用**
假设 `A` 和 `B` 互相持有对方的 `shared_ptr`，会导致资源无法释放。

**错误示例（会导致内存泄漏）**
```cpp
#include <iostream>
#include <memory>

struct B; // 先声明 B

struct A {
    std::shared_ptr<B> b_ptr;
    ~A() { std::cout << "A destroyed\n"; }
};
struct B {
    std::shared_ptr<A> a_ptr;
    ~B() { std::cout << "B destroyed\n"; }
};

void test() {
    auto a = std::make_shared<A>();
    auto b = std::make_shared<B>();
    a->b_ptr = b;
    b->a_ptr = a; // 形成循环引用
}

int main() {
    test();
} // test 作用域结束，a 和 b 的引用计数都不为0，导致资源泄漏
```
**修正方案：使用 `std::weak_ptr`**
```cpp
struct A {
    std::weak_ptr<B> b_ptr; // 改为 weak_ptr，避免循环引用
    ~A() { std::cout << "A destroyed\n"; }
};
struct B {
    std::shared_ptr<A> a_ptr;
    ~B() { std::cout << "B destroyed\n"; }
};
```
**适用场景**：
- **树状结构**（如 GUI 界面组件）。
- **父子关系**（如游戏中的对象管理）。
- **观察者模式**（`Observer` 监听 `Subject`）。

---

### **`std::unique_ptr` 的典型用法**
#### **1. 独占所有权（工厂模式）**
适用于 **独占资源管理**，如 **文件操作、网络连接、数据库句柄等**。

```cpp
#include <iostream>
#include <memory>

struct Database {
    Database() { std::cout << "Database connected\n"; }
    ~Database() { std::cout << "Database disconnected\n"; }
    void query() { std::cout << "Executing query...\n"; }
};

std::unique_ptr<Database> createDatabase() {
    return std::make_unique<Database>(); // 工厂模式返回 unique_ptr
}

int main() {
    std::unique_ptr<Database> db = createDatabase();
    db->query();
} // db 离开作用域，自动释放资源
```
**输出**
```
Database connected
Executing query...
Database disconnected
```
**适用场景**：
- 适用于**资源独占**的情况，如数据库、文件、网络连接等。
- **避免手动 `delete`**，自动释放资源。

---

#### **2. `std::unique_ptr` 结合 `std::move` 进行所有权转移**
```cpp
#include <iostream>
#include <memory>

struct Logger {
    Logger() { std::cout << "Logger created\n"; }
    ~Logger() { std::cout << "Logger destroyed\n"; }
};

void useLogger(std::unique_ptr<Logger> log) {
    std::cout << "Using Logger\n";
}

int main() {
    std::unique_ptr<Logger> logger = std::make_unique<Logger>();
    useLogger(std::move(logger)); // 转移所有权，logger 变为空

    if (!logger) {
        std::cout << "Logger moved and no longer owned\n";
    }
}
```
**输出**
```
Logger created
Using Logger
Logger destroyed
Logger moved and no longer owned
```
**适用场景**：
- 适用于 **对象生命周期较短** 且 **不需要共享** 的情况。
- 确保对象不会被多个地方同时持有，避免竞争条件（Race Condition）。

---

#### **3. 自定义删除器**
```cpp
#include <iostream>
#include <memory>

struct Foo {
    Foo() { std::cout << "Foo created\n"; }
    ~Foo() { std::cout << "Foo destroyed\n"; }
};

// 使用 lambda 作为自定义删除器
int main() {
    std::unique_ptr<Foo, void(*)(Foo*)> ptr(new Foo, [](Foo* f) {
        std::cout << "Custom deleter called\n";
        delete f;
    });
}
```
**输出**
```
Foo created
Custom deleter called
Foo destroyed
```
**适用场景**：
- 适用于**文件句柄、数据库连接等**，需要自定义释放逻辑的情况。

---

## **`shared_ptr` vs. `unique_ptr` 总结**
| 特性 | `std::shared_ptr` | `std::unique_ptr` |
|------|----------------|----------------|
| 所有权 | 允许多个 `shared_ptr` 共享 | 只能有一个 `unique_ptr` |
| 复制 | 可以复制（引用计数+1） | 不能复制 |
| 移动 | 允许 `std::move` 转移所有权 | 允许 `std::move` 转移所有权 |
| 循环引用 | 可能导致（需要 `weak_ptr` 解决） | 不会出现 |
| 适用场景 | 资源共享、动态对象生命周期复杂的场景 | 资源独占、工厂模式、临时资源管理 |
| 性能 | 需要维护引用计数，开销略大 | 轻量级，性能较优 |

### **何时使用哪种智能指针？**
| 场景 | 推荐指针 |
|------|---------|
| 资源独占（如数据库连接） | `std::unique_ptr` |
| 共享资源（如 GUI 组件） | `std::shared_ptr` |
| 解决 `shared_ptr` 的循环引用 | `std::weak_ptr` |
| 短生命周期、避免拷贝 | `std::unique_ptr` |
| 需要自定义释放逻辑 | `std::unique_ptr`（带删除器） |

**最佳实践**：
1. **默认使用 `std::unique_ptr`，避免不必要的共享。**
2. **只有在多个对象需要共享资源时，才使用 `std::shared_ptr`。**
3. **注意 `shared_ptr` 的循环引用问题，必要时使用 `weak_ptr`。**


## 右值引用

## 堆和栈区别

## 虚函数

## 多态
