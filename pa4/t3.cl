class C {
	a : Int;
	b : Bool;
	init(x : Int, y : Bool) : C {
           {
		a <- x;
		b <- y;
		self;
           }
	};
};

class D{
m: D;
u: SELF_TYPE;
};

class E inherits D {
    w: C <- new D;
    y: C <- self;
   i: SELF_TYPE <- self;
   o: SELF_TYPE <- new D;
   p: SELF_TYPE <- new E;
    t: E;
    q: SELF_TYPE;
r: Object;
    mmm(): Int {
	{
w <- (new C);
w <- (new Object);
w <- (new D);
let l: Int <- r in l + 3;

3;}
};
    init(x: C, y: Bool): E {
	{

let y: Int <- r + 2 in y + 3;
if 1 then 2 else 3 fi;
	     w <- x;
    	     w <- y;
 	     q <- (new C);
q <- (new D);
q <- (new SELF_TYPE);
 t;
m <- (new SELF_TYPE);
u <- m;
        }
    };
};

Class Main {
	main():C {
	 {
	  (new C).init(1,1);
	  (new C).init(1,true,3);
	  (new C).iinit(1,true);
	  (new C);
	 }
	};
};
