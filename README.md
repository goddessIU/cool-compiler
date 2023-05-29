1. 相关镜像存储在了阿里云盘 Compilers.rar就是我们最终版，解压后可以直接用的
另一个是初始版

pa2: 60/63, i can't figure the situation that, null in the string; I use the regular expression to solve it , but it doesn't 
work. the perfect answer which i copied from github is 63/63. The author of it not just use the simple regular expression
to solve the problem, he use some logical expression to solve it , and it does work. But i don't know why my solution 
doesn't work.
我的是60/63, 我应该是 null在字符串中的情况，处理不通过。 我是用了正则表达式，写了解决方案，却没有用。但从github找到
的满分答案，使用了c++ 中的while这些逻辑表达式，没单纯用正则解决字符串， 成功了。


pa3: i get 68/70, and I read the perfect.y, copied from github, and correct my solution, and now, the grade is 70/70.
the reason why first the gread is 68/70, is the order to deal with error. 
For example , first , 
class_list:
class_list class, 

class:
error

I omit many details, but i just wanna say, this is wrong. Because you first deal class_list, then class, it can't recover
from error and then to deal the next class, you should write in this form:
class_list:
class class_list

class:
error

or

class_list:
class_list class
| error class_list

这两种形式，如果一串class，构成class_list,比如第一个class 有问题，那就对应到了error，然后继续处理。
如果按照之前那样，那就是class_list可能需要不断左递归直到最后，第一个class对应上error，但是从第一个class_list的角度看，
它整个都是error。
感觉可能后面两种从直观感觉来看，更合理，因而也正确。第一种感觉上不是那么合理。
具体细节，我也不太明白，应该要去学习下bison底层原理和parser理论上如何对error进行处理，毕竟课上的DFA只处理了
无error的。

看cs143语法分析中视频，又感受了一下，其实error就是DFA卡住，然后一直抛弃，直到DFA可以再次运行；
所以右递归肯定行，比如上面，处理class_list, 正处在dfa相应状态，然后需要接受class，但是卡住了，然后抛弃，直到可以
继续。但是如果左递归，我还是不好说，希望将来再回顾的时候我可以搞懂吧！！！



pa4 74/74
总体上，还是有一些本地测试测不出来的小问题，可能是，我觉得；整体设计还可以，但是一些细节，因为我也不知道应该
在一些问题处理上具体怎么做，或者不同问题处理优先级（很详细的细节，pa上也没写），所以在这些地方就潦草了些，
但通过了全部测试

如果总复习的时候有时间，可以考虑再做做；
看了下别人的优秀实现，再反思下自己的：
1. 我理解的有些地方不到位。首先，错误的输出等没必要和官方的refsemant一模一样，比如顺序啊、输出啊，关键是要通过
error_semant，把错误数进行增加；我估计这也是官方判分的方法。 所以，很多地方因为以为要和官方输出的顺序内容一模一样，
导致浪费了大量时间、以及使得代码设计变乱
2. O\M两个环境，我用了非常复杂的map实现。这两个本质上是mapper， 其实用映射函数理解更合适。参考的答案里，就是
通过函数实现，并没有相对应的数据结构
3. 整体设计。整个lab分成三四部分，化整为零会很好，但是我没有站在全局考虑问题、设计数据结构和算法，而是一步步做，
type checking没想好，就写inheritance了。导致写完前两步，又要重构。还是要练。站在全局设计整体架构、数据结构、算法，
再化整为零的去实现。
4. 参考答案的Env设计很巧妙，将一些全局变量放入Env；既把相关数据结构抽离，又符合语义。
5. 对lab的不熟悉，比如把一切写在了classTable的构造函数里，看了参考答案，才知道原来可以写在下面。既是对lab不熟悉，
也是对任务没理解透彻，多多练习，多多写
6. 在整体设计好以后，我的局部实现出现了很多混乱，很大一部分和1有关，同时也说明细节设计还是有缺漏；过多次的
traverse， 没有整体思考好每一部分做哪些最合适；还有比如对SymbolTable没用好，像参考答案直接用它兜了个大底，而我
还用了一堆额外的map；以及官方的Class_这些就很好的保存了相关的type等，我又用了map记录，等等。
希望以后能提高整体设计能力，设计好了， 事倍功半；同时用好已有的（此处是官方提供的Class_ SymbolTable），也是很重要的
参考答案： 写的太好了 https://github.com/shootfirst/CS143/blob/main/assignments/PA4

5. pa5 63/63
整体设计不太好，很多地方有点面向测试了，最后shit山了（主要那个vim和操作系统实在难用，体验差，加上该期末了，
我就没精力了，就凑乎一下了）
然后gc没做，不过gc不做不影响分数，不知道为啥


