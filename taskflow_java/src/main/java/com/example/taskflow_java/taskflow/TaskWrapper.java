package com.example.taskflow_java.taskflow;

import lombok.Getter;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CompletableFuture;

@Getter
public class TaskWrapper {
    public TaskNode callable = null;
    public CompletableFuture<Boolean> future = null;
    public List<TaskWrapper> deps = new ArrayList<>();

    public TaskWrapper(TaskNode taskNode) {
        callable = taskNode;
    }

    public boolean run() {
        return callable.run();
    }
}
