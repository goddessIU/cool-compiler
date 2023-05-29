
(*  Example cool program testing as many aspects of the code generator
    as possible.
 *)
class A inherits IO{};

class B inherits A{};
class C inherits A{};
class Main {
   a : Int;
   main():Object   {
	{
           let d: B in 
	  d.out_string("abc");
	}
   };

};

