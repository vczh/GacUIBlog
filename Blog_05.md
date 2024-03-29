# GacUI：高效的布局系统 (2)

## 背景

上一篇文章 [高效的布局系统](./Blog_03.md) 到现在已经过去了三年，GacUI 的布局系统如今有了一个较大的翻新，主要是在实现上。

上一个版本的布局算法已经几乎实现了布局的局部刷新，以及按需触发布局的更新。但是依然存在着一个问题，就是布局的循环依赖。循环依赖的存在是需求导致的。比如`<SharedSizeRoot>`包含`<SharedSizeItem>`，但是同一个root下的item会互相同步尺寸。比如`<ResponseiveContainer>`包含各种responsive view，但是responsive view会因为container的大小的变化而改变自己的形状。循环依赖是不可能去掉的，但是实现上有一个问题，就是`GetBounds`和`GetMinSize`函数既要触发可能的布局刷新，而且布局的刷新也依赖其他composition的`GetBounds`和`GetMinSize`，这样调用来调用去就容易出问题。这个问题需要解决。

在这里介绍一下背景，`<SharedSizeRoot>`和`<SharedSizeItem>`通常被用在菜单和列表上。我们都知道，菜单项的图标、文字和快捷键需要列对齐，但是他们是一行一行的对象，不能做成表格。那具体如何对齐呢？GacUI的解决方法就是让菜单变成`<SharedSizeRoot>`，让文字和快捷键变成`<SharedSizeItem>`。item可以设置他的category，于是文字就知道他要跟其他文字对齐，快捷键就知道他要跟其他快捷键对齐。一轮布局下来，所有的文字被修改成一样的大小，所有的快捷键也被修改成一样的大小，自然就在视觉上列对齐了。

另一个问题则出现在列表控件的列表项布局上。熟悉GacUI的朋友们可能会意识到`GuiListControl`的`ItemArranger`属性，其实就是另一套布局，只不过是针对循环出现的对象以及生命周期的控制的。这样的设计的问题，就在于很难精准判断列表的布局什么时候应该发生。而且对于下拉列表，下拉的菜单大小跟列表项的大小是有关的，而且列表项的大小也是跟下拉菜单的大小有关的，这也形成了循环依赖，在实现上容易出问题。当然这个循环依赖依然是需求决定的，只能从实现入手。

最后一个问题则在于data binding，虽然GacUI不鼓励把data binding应用在尺寸上，但是有的时候确实是没办法。这样也会创建我作为GacUI的作者完全不可预知的循环依赖，搞不好容易因为使用者的data binding写得不好而出现死循环，对于使用者来说这显然是一个难以debug的问题。

那有没有可能从根本上解决这些麻烦呢？

## 改变 1

1.0 版本遗留下来的这几个问题，在 1.2.7.0 和 1.2.8.0 两个版本分别得到了解决，此时已经过去了两年半。

我对第一个问题的解决方法，就是引入布局的两阶段更新。此前说到布局的触发是由`GetBounds`和`GetMinSize`发生的，而你调用`GetBounds`和`GetMinSize`通常有两个原因，一个是你需要知道对象的尺寸，另一个是你在改自己的布局所以需要对依赖进行求解。`GetBounds`不知道你到底是出于什么目的再调用，于是才造成了麻烦。那解决的问题很简单，那就是把他们分开。

怎么分开呢？它们之间的主要区别，就是对依赖进行求解的时候，其实你不需要知道对象最新的尺寸，因为这个尺寸有可能是在布局的过程中算了一半的。而且在求解后对象的尺寸被改了，那还会在下一帧触发新一轮的求解，很快就收敛了。那解决起来就很简单了，我不应该提供`GetBounds`，改为了`GetCachedBounds`和和`GetCachedMinSize`，其结果是上一轮布局更新后的结果。

每一轮布局的时候会分成两步。

第一步是更新`GetCachedMinSize`的结果。一个对象的`GetCahcedMinSize`通常依赖于他的子对象的`GetCachedMinSize`。而特殊的对象可以把他的`GetCachedMinSize`的计算托管给父对象，比如`<Cell>`对`<Table>`，`<StackItem>`对`<Stack>`，`<FlowItem>`对`<Flow>`等等。等子对象的`GetCachedMinSize`更新完就轮到父对象的`GetCachedMinSize`，这样父对象已经掌握了所有子对象的`GetCachedMinSize`的情况，也可以计算自己的`GetCachedMinSize`了。

第二步是更新`GetCachedBounds`的结果。`GetCachedBounds`的顺序则是相反的，先算父对象，再算子对象。具体原因就不详细描述了，是由依赖关系的求解决定的。每一个对象把自己的算完，先不会更新`GetCachedBounds`的结果，而会把这个结果先给子对象算他自己的`GetCachedBounds`，最后再回来更新自己的结果。而特殊的对象可以把他的`GetCachedBounds`的计算托管给父对象。

通过这样的设计，每一轮的布局依赖的都是上一轮布局的结果。在有些情况下可能一轮计算下来不能计算出正确的结果，不过马上就会有下一轮，很快就收敛了。一旦GacUI发现布局收敛，它就不会再算下一轮了，于是接下来的每一帧都什么都不做，不算布局，不刷新，直到下次UI做出改变。

## 改变 2

而`GuiListControl`和`ItemArranger`属性，我最终并没有去掉，但是我把每一个ItemArranger的计算都托管给了为他们特别创造的列表项布局对象（基类是`GuiVirtualRepeatCompositionBase`）里面，从而把列表项的布局纳入上面的系统里。最新的文档里面已经有这些新的布局对象的介绍，他们的主要功能是data binding，你把一个容器和对象皮肤给它们，他们会帮你处理布局的同时，不会为了看不见的东西浪费资源。你对他们进行滚动，新出现的项会构造皮肤实例，而旧的项会删除对应皮肤实例。

布局的计算跟上面的系统描述的一样，先算出所有项的`GetCachedMinSize`，然后由容器统一安排他们的位置。然后我写了一个模板类，这样你就可以从一个列表项布局对象类直接构造出对应的ItemArranger，交给`GuiListControl`使用。

## 改变 3

第三个问题，是如何让使用者的data binding也纳入到这流程当中呢？我找到了一个非常简单的解决方法，就是让`CachedMinSize`和`CachedBounds`变成只读的，这样他们就不能被绑定了。但是我同时提供了`PreferredMinSize`和`ExpectedBounds`两个可以写的属性。使用者的data binding只会把其他对象的`CachedBounds`、`CachedMinSize`等属性的结果更新到这个对象的`PreferredMinSize`和`ExpectedBounds`属性，而写入这两个对象并不会直接改变对象的尺寸和位置，而会在下一帧触发布局的计算，从而把这两个属性的纳入运算，从而更新最终的结果。

## 尾声

GacUI对布局的分解无疑是成功的，这套接口这么多年都没有变过。但是实现已经写了第三遍。不过这最后一次的更新留下了一个小问题，就是需要两轮布局才能收敛的情况变多了，其结果就是对于高刷新率的显示器，布局了一半的结果也会被画出来一次。有些有强迫症的人可能会觉得难受。为此我给窗口和布局对象都提供了`ForceCalculateSizeImmediately`函数，如果有些操作带来了一些稳定复现的、令你觉得很难受的刷新问题，那你只要调用一下这个函数，他就会马上再算一遍，这样就不会看到第一轮的结果了。

GacUI到现在已经是13年了，我自己也用了很久，虽然GacUI的布局设计在正交度上是相当成功的，但是使用过程仍然有点繁琐。我会在以后推出一个GacUI XML Resource专用的facade，针对很多常用的情况直接给你生成我认为最好的布局方式，一遍省去新用户大量的打字以及积累经验的时间。