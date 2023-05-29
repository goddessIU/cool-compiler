
class C {
        l : SELF_TYPE;
	a : Int;
	b : Bool;
	init(x : Int, y : Bool) : C {
           {
                case 3 of 
                d: Int => 2;
                esac;
		a <- x;
		b <- y;
		self;
           }
	};
};

Class Main {
	main():C {
	  (new C).init(1,true)
	};
};
