#!/usr/bin/python3

class A:
    def a(self):
        print("Aa")
    def b(self):
        print("Ab")
    def c(self):
        print("Ac")

class B(A):
    def b(self):
        super().b()
        print("Bb")
    def c(self):
        super().c()
        print("Bc")
    def d(self):
        print("Bd")

class C(A):
    def c(self):
        super().c()
        print("Cc")
    def d(self):
        print("Cd")
    def e(self):
        print("Ce")

class D(B,C):
    def a(self):
        print("--- d.a() ---")
        super().a()
        print("Da")
    def b(self):
        print("--- d.b() ---")
        super().b()
        print("Db")
    def c(self):
        print("--- d.c() ---")
        super().c()
        print("Dc")
    def d(self):
        print("--- d.d() ---")
        super().d()
        print("Dd")
    def e(self):
        print("--- d.e() ---")
        super().e()
        print("De")

print(D.mro())
d = D()
d.a()
d.b()
d.c()
d.d()
d.e()
