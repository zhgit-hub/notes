- [MYSQL](#mysql)
  - [MyISAM与InnoDB的区别](#myisam与innodb的区别)
  - [InnoDB的关键特性](#innodb的关键特性)
    - [Insert Buffer插入缓冲](#insert-buffer插入缓冲)
    - [Double Write两次写](#double-write两次写)
    - [Adaptive Hash Index自适应哈希索引](#adaptive-hash-index自适应哈希索引)
    - [Async IO异步IO](#async-io异步io)
    - [Flush Neighbor Page刷新邻接页](#flush-neighbor-page刷新邻接页)
  - [InnoDB逻辑存储结构](#innodb逻辑存储结构)
  - [索引](#索引)
    - [聚集索引](#聚集索引)
    - [非聚集索引](#非聚集索引)
    - [联合索引](#联合索引)
    - [覆盖索引](#覆盖索引)
    - [自适应哈希索引](#自适应哈希索引)
    - [索引失效](#索引失效)
    - [索引底层优化](#索引底层优化)
  - [锁](#锁)
    - [lock、latch](#locklatch)
    - [共享锁(S Lock)与排他锁(X Lock)](#共享锁s-lock与排他锁x-lock)
    - [意向锁](#意向锁)
  - [数据完整性](#数据完整性)
  - [InnoDB的后台线程分类](#innodb的后台线程分类)
  - [InnoDB的内存管理](#innodb的内存管理)
    - [缓冲池](#缓冲池)
    - [LRU List](#lru-list)
    - [Free List](#free-list)
    - [Flush List](#flush-list)
    - [Check Point机制](#check-point机制)
  - [视图](#视图)
  - [分区](#分区)
- [REDIS](#redis)

#   MYSQL
##  MyISAM与InnoDB的区别
|   MyISAM  |   InnoDB  |   
|    :---:      |    :---:      |
|   不支持事务  |   支持事务    |
|   不支持外键  |   支持外键    |
|   索引的叶子节点存放的是数据文件的地址指针  |   聚集索引的叶子节点存放的是磁盘上实际的数据文件，非聚集索引的叶子节点存放的是主键值    |
|   支持表级锁  |   支持表级锁与行级锁  |
|   可以没有主键  |   必须要有主键，如果不主动设置会默认产生一个  |

##  InnoDB的关键特性    
### Insert Buffer插入缓冲
使用条件 : 非聚集且不唯一的索引   
在进行插入操作时，插入顺序很可能不是顺序的，如果不使用InsertBuffer则需要离散的访问非聚集索引页(随机读取的性能低于顺序读取)。    
使用InsertBuffer后，先判断插入或更新的非聚集索引页是否存在于InsertBuffer中，如果存在则直接插入，如果不存在则先加入到一个InsertBuffer中。之后在一定的频率和条件下进行InsertBuffer与非聚集叶子节点的合并操作。    
缺点 : 进行大量的非聚集且不唯一的索引的插入操作时(使用了大量InsertBuffer)，若此时数据库发生了宕机，会有很多InsertBuffer没有合并到非聚集索引中去，恢复会需要很长时间。   
Change Buffer : InsertBuffer的升级，包含Insert Buffer(INSERT、UPDATE)、Delete Buffer(DELETE)、Purge Buffer。使用条件仍为非聚集且不唯一的索引。一阶段使用InsertBuffer或DeleteBuffer将记录标记为删除，二阶段使用PurgeBuffer将记录真正删除。

### Double Write两次写
部分写失效 : 正在从内存往磁盘写入页时发生数据库宕机，只写了一部分。当页本身发生了损坏，是无法通过redo日志进行重做恢复的，因为redo日志记录的是对页的物理操作。   
<!-- ![两次写](https://github.com/zhgit-hub/notes/raw/master/image/DoubleWrite.png)      -->
![两次写](./image/DoubleWrite.png) 
两次写机制由两部分组成 : 内存中的2MB大小的DoubleWriteBuffer与物理磁盘上共享表空间中一共2MB大小的128个连续的DoubleWrite页。    
对缓冲池的脏页进行刷新时，先将脏页复制到内存中的DoubleWriteBuffer，然后通过DoubleWriteBuffer分两次每次1MB顺序写到磁盘上共享表空间中的DoubleWrite页，最后将DoubleWriteBuffer离散地写入到各个目的表空间中。   
当把页写入磁盘时发生了崩溃，恢复过程中可以在共享表空间中的DoubleWrite中找到页的副本，通过该副本来还原损坏的页，再进行重做。

### Adaptive Hash Index自适应哈希索引
InnoDB会自动根据访问的频率和模式来自动地为某些热点页建立哈希索引。    
自适应哈希索引通过缓冲池的B+树页构建而来，因此建立的速度很快，而且不需要对整张表构建哈希索引。

### Async IO异步IO
使用异步IO来避免IO的等待时间以及进行合并多个IO操作

### Flush Neighbor Page刷新邻接页
当刷新一个脏页时会检测该页所在区的所有页，若有脏页则一并刷新。可以通过AIO将多个IO操作合并为一个IO操作。

##  InnoDB逻辑存储结构
<!-- ![逻辑存储结构](https://github.com/zhgit-hub/notes/raw/master/image/StorageStructure.png)   -->
![逻辑存储结构](./image/StorageStructure.png)  
表空间-段-区-页-行

##  索引
### 聚集索引
表中数据以主键顺序存放，聚集索引按照每张表的主键构建一棵B+树，叶子节点中存放表的行记录数据，也将聚集索引的叶子节点称为数据页。    
查询优化器倾向于使用聚集索引:
+ 可以在聚集索引的叶子节点上直接找到数据
+ 聚集索引定义了数据的逻辑顺序，能够快速地进行针对范围值地查询    

### 非聚集索引
叶子节点存储行数据的聚集索引键。通过非聚集索引查找数据时，先找到行数据在非聚集索引上的叶子节点，再通过叶子节点中存储的主键去聚集索引上查找一个完整的行记录(回表查询)。

### 联合索引
联合索引是指对表上的多个列进行索引。可以只使用联合索引的左侧连续的一部分索引进行查询(最左前缀匹配原则)。    
好处:
+ 联合索引对后面的键值也进行了排序处理。
+ 可以覆盖索引。

### 覆盖索引
从非聚集索引中就可得到查询的记录，而不需要查询聚集索引中的记录。  
好处:    
+ 通过使用覆盖索引可以减少大量的IO操作，因为非聚集索引不包含整行记录的所有信息，大小要小于聚集索引。

### 自适应哈希索引

### 索引失效
+ 未遵守最左匹配原则
+ 使用不等于(!=或<>)
+ 在索引上做计算、函数、类型转换
+ 以%开头的like查询
+ 使用了or且or条件中的列不全有索引
+ 范围条件右边的列
+ 使用is null/is not null判断
+ 索引字段为字符串，但查询时未加单引号
+ 当全表搜索比使用索引快时
  
### 索引底层优化
+ Multi-Range Read:根据辅助索引进行范围查询(range,ref,eq_ref类型)时，将查询到的辅助索引键值存放于一个缓存中(此时根据辅助索引键值排序)，然后将缓存中的键值根据rowid进行排序，最后根据排序后的键值进行访问实际的数据文件。MRR使得范围查询以主键顺序进行回表查找，同时减少了缓冲池中页被替换的次数。     
+ Index Condition Pushdown:在取出索引的同时，判断是否可以进行WHERE条件的过滤(将WHERE的部分过滤操作放在了存储引擎层)。

##  锁
### lock、latch
![锁](./image/LockAndLatch.png)   

### 共享锁(S Lock)与排他锁(X Lock)
![共享锁-排他锁](./image/SXLock.png)   
共享锁与排他锁都是行锁。      

### 意向锁
InnoDB支持表级意向锁。
意向共享锁:事务想要获得一张表中某几行的共享锁。     
意向排他锁:事务想要获得一张表中某几行的排他锁。
![兼容性](./image/IntentLock.png)     

##  数据完整性
约束 : 
+ Primary Key
+ Unique Key
+ Foreign Key
+ Default
+ Not Null

实体完整性 : 保证表中有一个主键。
+ 通过Primary Key或Unique Key    
+ 编写触发器    

域完整性 : 保证数据每列的值满足特定的条件。   
+ 选择合适的数据类型确保一个数据值满足特定条件
+ Foreign Key或Default
+ 编写触发器    
    
参照完整性 : 保证两张表之间的关系   
+ Foreign Key
+ 编写触发器 

##  InnoDB的后台线程分类
1、Master Thread    
负责将缓冲池中的数据异步刷新到磁盘，保证数据的一致性  
2、IO Thread  
使用异步IO(AIO)来处理IO请求   
3、Purge Thread   
回收已经使用并分配的undo页  
4、Page Cleaner Thread    
负责脏页的刷新

##  InnoDB的内存管理
### 缓冲池   
<!-- ![缓冲池](https://github.com/zhgit-hub/notes/raw/master/image/BufferPool.png)    -->
![缓冲池](./image/BufferPool.png)
缓冲池就是一块内存区域，用来弥补CPU与磁盘间速度的鸿沟。   
读取页时，首先会检查缓冲池，如果缓冲池中有，则缓冲命中，直接读取缓冲池中的数据页。如果缓冲池中没有，则读取磁盘中的数据页，并把其存放在缓冲池中。  
修改页时，会修改位于缓冲池中的页，此时这种页被称为"脏页"，脏页会以一定的频率一起刷新到磁盘上。

### LRU List
使用LRU链表来管理缓冲池中已使用的页。InnoDB对其做了相应的改进，从磁盘上最新读取的页不是直接插入链表头部，而是链表中间midpoint的地方，midpoint之前的数据为活跃的数据(热端，默认大小5/8)，之后的数据为相对不活跃的数据(冷端，默认大小3/8)，通过设置innodb_old_blocks_time可以让冷端数据等待一段时间之后再次被访问才能移到热端。   
目的：全局扫描与预读到的页可能不是热点数据，直接插入LRU链表头可能会把真正的热点数据置换出去。  

### Free List
使用Free链表来管理缓冲池中未使用的页。当缓冲池需要使用页时，首先从Free链表上查询是否有可用的页，如果有则从Free链表上取走一页，放入LRU链表上。如果没有，则根据LRU算法淘汰LRU列表末尾的一页分配给新页。

### Flush List    
使用Flush链表来管理需要刷新回磁盘的脏页，脏页实际仍然存放在LRU链表。当LRU中的页被修改后成为脏页，就将该页加入Flush列表，通过CheckPoint机制按列表顺序刷新回磁盘。

### Check Point机制
当数据库宕机时，只需对CheckPoint之后的redo日志进行重做，之前的脏页已经刷新回磁盘。      
1、Sharp CheckPoint     
将所有脏页都刷新回磁盘    
时机 :    
+ 数据库关闭时产生CheckPoint。  
     
2、Fuzzy CheckPoint
将部分脏页刷新回磁盘    
时机 :      
+ Master Thread CheckPoint : Master线程每隔1s或10s异步刷新Flush链表，产生CheckPoint      
+ Flush_LRU_List CheckPoint : 当Free链表少于100时，将LRU链表尾部的页移除，若有脏页则产生CheckPoint     
+ Async/Sync Flush CheckPoint : 当redo日志使用率超过Async/Sync阈值时为了循环使用redo日志，产生CheckPoint     
+ Dirty Page too much : 脏页数目太多，为了确保缓冲池有足够可用的页，产生CheckPoint   


##  视图
一个命名的虚表，可以当作表来使用，但是没有实际的物理存储。

##  分区
水平分区 : 将同一表中的不同行的记录分配到不同的物理文件中   
垂直分区 : 将同一表中的不同列的记录分配到不同的物理文件中   

局部分区 : 一个分区中既存放了数据又存放了索引   
全局分区 : 数据存放在各个分区中，所有数据的索引放在一个对象中   

MySQL支持水平分区与局部分区。
分区不一定能够提升性能，数据库的应用分为两类:OLTP(在线事务处理，如Blog、网络游戏等，一般只会通过索引获取几条数据)、OLAP(在线分析处理，如数据仓库，大多数查询需要频繁地扫描一张很大的表)。OLTP一般不需要分区，OLAP分区可以提升查询的性能。~~如果查询语句走非聚集索引且不走分区键，将会遍历所有分区，大大降低性能。~~




#   REDIS