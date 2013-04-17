# lq

Command-line Labeled Tab-separated Values (LTSV) processor

## Dependencies

* [ltsv4c](https://github.com/9re/ltsv4c)

## Build

```
% cmake .
% make
% make install
```

## Example

test.txt:

```
a:2013/04/17 12:00:00 +0900	b:x.x.x.x	c:hoge
b:2013/04/17 12:01:00 +0900	c:x.y.x.y	a:foo	d:new
```

Output of test.txt:

```
% lq -f test.txt
--
a: 2013/04/17 12:00:00 +0900
b: x.x.x.x
c: hoge
--
b: 2013/04/17 12:01:00 +0900
c: x.y.x.y
a: foo
d: new
--
```

Output of the label 'a' and 'b' in test.txt:

```
% lq -f test.txt -l a,b
--
a: 2013/04/17 12:00:00 +0900
b: x.x.x.x
--
a: foo
b: 2013/04/17 12:01:00 +0900
--
```

Output of the label 'c' and 'd' in test.txt:

```
% lq -f test.txt -l c,d
--
c: hoge
--
c: x.y.x.y
d: new
--
```

```
% lq -f test.txt -l c,d -v
--
c: hoge
d: (null)
--
c: x.y.x.y
d: new
--
```

Output of stdin:

```
% cat test.txt | lq
--
a: 2013/04/17 12:00:00 +0900
b: x.x.x.x
c: hoge
--
b: 2013/04/17 12:01:00 +0900
c: x.y.x.y
a: foo
d: new
--
```
