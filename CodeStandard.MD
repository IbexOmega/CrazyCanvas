## Code Standard

#### Contents
* If-Statements
* Loops
* Templates
* Platform Specific Code
* Naming Conventions

### If-Statements
* If statements should be written using the following form:

```
if (condition)
{
  //Statements
}
else
{
  //Statements
}
```
* On small codesegments following example is allowed but the above is prefered
```
if (condition)
  return true
```
* Conditional (ternary) operator are allowed
```
condition ? true : false;
```

### Loops
* Loops (for, while, do while) should be written using the following form:

```
while (condition)
{
  //Statements
}
```
* On small codesegments following example is allowed but the above is prefered
```
while (condition)
  //Statements
```
* for- and while-loops are preferred but do-while is allowed

### Templates
* Avoid templates as much as possible to avoid longer compile times. 
If used, the following form should be used:
```
template<typename T>
class TMyClass
{
};
```


### Classes
* A single class per header should be used, unless the types are strongly connected like an enum or descriptor.
```
struct TextureDesc
{
};

class Texture
{
};
```
* Access Modifers should appear in the following order:
```
class MyClass
{
//Member functions
public:
  void Func();
protected:
  void Func();
private:
  void Func();

//Static functions
public:
  static void Func();
protected:
  static void Func();
private:
  static void Func();
  
//Member variables
public:
  int MyInteger;
protected:
  int m_MyInteger;
private:
  int* m_pMyPointer;
  
//Static variables
public:
  static int Integer;
protected:
  static int m_Integer;
private:
  static int* m_pPointer;
};
```
* Header files should be included in compilation units (.cpp). Only reason to include in the header is when a class/struct is 
using the type not as a pointer or reference. This means that forward declarations has to be used.

* Classes that are meant to used outside the engine- project, and are not header-only should use the DLL-export directive before the class-name
```
class LAMBDA_API MyClass
{
};
```

### Naming Conventions
* Pointers should use the letter 'p' as prefix 
'''
int*   pPointer  = nullptr;
int**  ppPointer = nullptr;
'''

* Private/Protected members should use the letter 'm_' as prefix 
'''
int    m_MyInteger = 0;
int*   m_pPointer  = nullptr;
int**  m_ppPointer = nullptr;
'''

* Static variables should use the letter 's_' as prefix 
'''
static int    s_Integer = 0;
static int*   s_pPointer  = nullptr;
static int**  s_ppPointer = nullptr;

void Func()
{
  static int s_LocalVariable = 0;
}
'''

* Globals **(Should be avoided if possible)** should use the letter 'g_' as prefix
'''
int    g_MyInteger = 0;
int*   g_pPointer  = nullptr;
int**  g_ppPointer = nullptr;
'''

* Functions use pascalcase. Parameters and local variables use camelcase
```
void FunctionsLookLikeThis(int thisIsAParameter, int* pPointerParameter)
{
  int thisIsALocalVariable = thisIsAParamter;
}
```

### Interfaces
* Interfaces do not contain any state (i.e no variables) and does not provide any functiondefinition. All functions should be pure virtual.

* Interfaces should use the capital letter 'I' as prefix.

* Interfaces should use the declare interface macro

```
class IMyInterface
{
public:
  DECL_INTERFACE(IMyInterface);
  virtual void Func() = 0;
}
```

### Enums
* Since enums are constants, they should be written in capital letters
* Enums should use the capital letter 'E' as prefix.
* Enum class should be preferred, unless they mostly will be used as integers, like flags
```
enum class EMyEnum : uint16
{
  ENUM_1
};
```

* Enums **not** using the class keyword should prefix all enumerators with the name of the enum 
```
enum EMyFlags
{
  MY_FLAG_FLAG = (1 << 0),
  MY_FLAG_FLAG = (1 << 1),
};
```

### Struct
* 
### Platform Specific Code




