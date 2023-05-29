class C inherits D{
	a : Int;
	b : Bool;
        u : M;
	init(x : R) : N {
           {
		a <- x;
		b <- y;
		3;
           }
	};
};

class D{};

Class Main {
	main():C {
	 {
	  (new C).init(1,1);
	  (new C).init(1,true,3) + 3;
	  (new C).iinit(1,true) + 3;
	  (new C)@E.init(1,1) + 2;
          (new C)@D.init(1, true);
          (new C)@D.init(1, true, 3);
          (new C)@SELF_TYPE.iinit(1, true) + 3;
          (new C).init(1, 1, 3); 
	  (new C)@D.init() + 2; 
   }
	};
};
