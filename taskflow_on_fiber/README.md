# 简介
基于boost::fiber实现的类似taskflow的图引擎。
相对于taskflow，的优势:
- 图关系与运行关系分离，graph对象可并行执行
- 使用了boost::fiber，可处理既有IO又有计算的任务

# 依赖
依赖boost1.66，需要boost.fiber使用

