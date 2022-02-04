package com.example.taskflow_java.taskflow;

import lombok.Data;

import java.util.ArrayList;
import java.util.List;

/**
 * 单个任务的配置，包括执行的类及其依赖
 */
@Data
public class TaskConfig {
    /**
     * 任务名，全路径
     */
    String task = "";
    /**
     * 依赖的任务，全路径
     */
    List<String> dependencies = new ArrayList<>();
}
