#ifdef tests_here

#include <iostream>

#define CATCH_CONFIG_MAIN
#include "catch_amalgamated.hpp"

#include "state.hpp"
#include "flag_group.hpp"

enum MyEnum {
	A,
	B,
	C,
	D,
	E,
};

enum class MyEnumClass {
	A,
	B,
	C
};

TEST_CASE() {
	state<MyEnum> e { A };
	e = B;
	REQUIRE(e.is(B) == true);
	REQUIRE(e.is(A, B) == true);
	REQUIRE(e.is(C) == false);
	REQUIRE(e.is(A, C) == false);
}

TEST_CASE() {
	state<MyEnum> e { A };
	e = B;
	state<MyEnum> f { e };
	REQUIRE(f.is(B) == true);
	REQUIRE(f.is(A, B) == true);
	REQUIRE(f.is(C) == false);
	REQUIRE(f.is(A, C) == false);
}

TEST_CASE() {
	state<MyEnum> e { A };
	e = B;
	state<MyEnum> f {};
	f = e;
	REQUIRE(f.is(B) == true);
	REQUIRE(f.is(A, B) == true);
	REQUIRE(f.is(C) == false);
	REQUIRE(f.is(A, C) == false);
}

TEST_CASE() {
	state<MyEnumClass> e { MyEnumClass::A };
	e = MyEnumClass::B;
	REQUIRE(e.is(MyEnumClass::B) == true);
	REQUIRE(e.is(MyEnumClass::A, MyEnumClass::B) == true);
	REQUIRE(e.is(MyEnumClass::C) == false);
	REQUIRE(e.is(MyEnumClass::A, MyEnumClass::C) == false);
}

TEST_CASE() {
	state<MyEnumClass> e { MyEnumClass::A };
	e = MyEnumClass::B;
	state<MyEnumClass> f { e };
	REQUIRE(f.is(MyEnumClass::B) == true);
	REQUIRE(f.is(MyEnumClass::A, MyEnumClass::B) == true);
	REQUIRE(f.is(MyEnumClass::C) == false);
	REQUIRE(f.is(MyEnumClass::A, MyEnumClass::C) == false);
}

TEST_CASE() {
	state<MyEnumClass> e { MyEnumClass::A };
	e = MyEnumClass::B;
	state<MyEnumClass> f {};
	f = e;
	REQUIRE(f.is(MyEnumClass::B) == true);
	REQUIRE(f.is(MyEnumClass::A, MyEnumClass::B) == true);
	REQUIRE(f.is(MyEnumClass::C) == false);
	REQUIRE(f.is(MyEnumClass::A, MyEnumClass::C) == false);
}

TEST_CASE() {
	flag_group<MyEnum> fg {};
	fg.raise(B, A);
	fg.raise(C);
	REQUIRE(fg.has(A, B, C) == true);
	fg.drop(B, A);
	REQUIRE(fg.has(A) == false);
	REQUIRE(fg.has(A, B) == false);
	REQUIRE(fg.has(B) == false);
	REQUIRE(fg.has(C) == true);
}

TEST_CASE() {
	flag_group<MyEnum> fg {};
	fg.raise(B, A);
	fg.drop(B);
	flag_group<MyEnum> fg2 { fg };
	REQUIRE(fg2.has(A) == true);
	REQUIRE(fg2.has(A, B) == false);
	REQUIRE(fg2.has(B) == false);
}

TEST_CASE() {
	flag_group<MyEnum> fg {};
	fg.raise(B);
	fg.raise(A);
	fg.drop(B);
	flag_group<MyEnum> fg2 {};
	fg2 = fg;
	REQUIRE(fg2.has(A) == true);
	REQUIRE(fg2.has(A, B) == false);
	REQUIRE(fg2.has(B) == false);
}

TEST_CASE() {
	flag_group<MyEnum> fg {};
	flag_group<MyEnum> fg2 { fg };
	fg2.raise(C);
	REQUIRE(fg.has(C) == false);
	REQUIRE(fg2.has(C) == true);
}

TEST_CASE() {
	flag_group<MyEnum> fg {};
	flag_group<MyEnum> fg2 {};
	fg2 = fg;
	fg2.raise(C);
	REQUIRE(fg.has(C) == false);
	REQUIRE(fg2.has(C) == true);
}

TEST_CASE() {
	flag_group<MyEnum> fg {};
	fg.raise(B, A);
	fg.drop(B);
	fg.clear();
	REQUIRE(fg.has(A) == false);
	REQUIRE(fg.has(A, B, C) == false);
}

TEST_CASE() {
	flag_group<MyEnumClass> fg {};
	fg.raise(MyEnumClass::B, MyEnumClass::A);
	REQUIRE(fg.has(MyEnumClass::A, MyEnumClass::B) == true);
	fg.drop(MyEnumClass::B);
	REQUIRE(fg.has(MyEnumClass::A) == true);
	REQUIRE(fg.has(MyEnumClass::A, MyEnumClass::B) == false);
	REQUIRE(fg.has(MyEnumClass::B) == false);
}

TEST_CASE() {
	flag_group<MyEnumClass> fg {};
	fg.raise(MyEnumClass::B, MyEnumClass::A);
	fg.drop(MyEnumClass::B);
	flag_group<MyEnumClass> fg2 { fg };
	REQUIRE(fg2.has(MyEnumClass::A) == true);
	REQUIRE(fg2.has(MyEnumClass::A, MyEnumClass::B) == false);
	REQUIRE(fg2.has(MyEnumClass::B) == false);
}

TEST_CASE() {
	flag_group<MyEnumClass> fg {};
	fg.raise(MyEnumClass::B, MyEnumClass::A);
	fg.drop(MyEnumClass::B);
	flag_group<MyEnumClass> fg2 {};
	fg2 = fg;
	REQUIRE(fg2.has(MyEnumClass::A) == true);
	REQUIRE(fg2.has(MyEnumClass::A, MyEnumClass::B) == false);
	REQUIRE(fg2.has(MyEnumClass::B) == false);
}

TEST_CASE() {
	flag_group<MyEnumClass> fg {};
	flag_group<MyEnumClass> fg2 { fg };
	fg2.raise(MyEnumClass::C);
	REQUIRE(fg.has(MyEnumClass::C) == false);
	REQUIRE(fg2.has(MyEnumClass::C) == true);
}

TEST_CASE() {
	flag_group<MyEnumClass> fg {};
	flag_group<MyEnumClass> fg2 {};
	fg2 = fg;
	fg2.raise(MyEnumClass::C);
	REQUIRE(fg.has(MyEnumClass::C) == false);
	REQUIRE(fg2.has(MyEnumClass::C) == true);
}

TEST_CASE() {
	flag_group<MyEnumClass> fg {};
	fg.raise(MyEnumClass::B, MyEnumClass::A);
	fg.drop(MyEnumClass::B);
	fg.clear();
	REQUIRE(fg.has(MyEnumClass::A) == false);
	REQUIRE(fg.has(MyEnumClass::A, MyEnumClass::B, MyEnumClass::C) == false);
}

#endif