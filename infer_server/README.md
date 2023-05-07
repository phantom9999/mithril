# 简介

功能:
- 模型管理 done
  - 支持多模型、支持多版本 done
  - 支持多版本热加载 done
  - 模型请求校验 done
  - 模型加载策略 done
    - 指定版本加载 done
    - 全加载 done
    - 最新模型加载 done
- 服务 done
  - grpc推理接口 done
  - grpc模型状态接口 done
- 监控 done
  - 服务维度监控 done
  - 模型维度监控 done
  - 监控配置化 done
- batch doing
  - 多模型批次推理 done
  - 自适应批次推理 todo
- 模型实验 done
  - 根据版本获取模型 done
  - 根据标签获取模型 done
- 其他功能
  - 预热 done

# 结构
模型目录结构:
- model_name1
  - model.pt 模型导出的torch script
  - feature.txt 特征描述文件
  - _SUCCESS 检测文件
  - md5.txt md5文件
  - warmup.bin 预热文件(二进制)
