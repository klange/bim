# This is a Markdown test file.

It contains several different languages.

## C/C++

```c
#include <stdio.h>
int main(int argc, char * argv[]) {
	fprintf(stderr, "hello, %s!\n", "world");
	return 0;
}
```

## Kuroko

```krk
from syntax import Highlighter

class KrkHighlighter(Highlighter):
    def foo():
        self.bar()

f'{test.string()}'

'''It was the best of times,
it was the blurst of times.'''

and this is not actually valid code

```

## Python

```py
#!/usr/bin/env python
"""Hello, world"""

@SomeDecorator
class Foo(object):
	def __init__(self):
		print("Hello, world!")
```

## Java

```java
/**
 * Hello, World!
 * @author Some person.
 */
public class HelloWorld {
	public static void main(String[] args) {
		System.out.println("Hello, World");
	}
}
```

## Diff

```diff
diff --git a/bim.c b/bim.c
index e9df28c..344ca40 100644
--- a/bim.c
+++ b/bim.c
@@ -41,7 +41,7 @@
 #include <sys/ioctl.h>
 #include <sys/stat.h>
 
-#define BIM_VERSION   "1.4.3"
+#define BIM_VERSION   "1.4.4"
 #define BIM_COPYRIGHT "Copyright 2012-2019 K. Lange <\033[3mklange@toaruos.org\033[23m>"
 
 #define BLOCK_SIZE 4096
```

## JSON

```json
{
	"string": "value",
	"numeral": 123.4,
	"boolean": false,
	"array": []
}
```

## XML

```xml
<foo>
	<bar baz="bix">Hello!</bar> World.
</foo>
```

## HTML

```html
<!doctype html>
<html>
	<head>
		<title>Hello, World!</title>
		<style>
			body {
				color: #123456;
				text-decoration: underline;
			}
		</style>
	</head>
	<body>
		<p>Hello, world!</p>
	</body>
</html>
```

## Make

```make
CC=gcc
all: foo

foo: foo.c
	${CC} -o $@ $<
```

## Rust

```rust
/* Comment /* Nesting */ */
fn main() {
	// Regular comment
	println!("Hello World!");
}
```

Thanks for reading!
