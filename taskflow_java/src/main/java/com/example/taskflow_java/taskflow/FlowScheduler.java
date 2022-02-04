package com.example.taskflow_java.taskflow;

import com.google.common.graph.GraphBuilder;
import com.google.common.graph.Graphs;
import com.google.common.graph.MutableGraph;
import lombok.extern.log4j.Log4j2;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.ApplicationContext;
import org.springframework.stereotype.Component;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.stream.Collectors;

@Log4j2
@Component
public class FlowScheduler {
    @Autowired
    ApplicationContext applicationContext;

    ExecutorService executorService = Executors.newWorkStealingPool();

    final Set<String> taskSet;

    public FlowScheduler(List<TaskNode> taskNodeList) {
        taskSet = taskNodeList.stream()
                .map(taskNode -> taskNode.getClass().getName())
                .map(task->task.split("\\$\\$")[0])
                .collect(Collectors.toSet());
    }

    public boolean runWithoutThrow(FlowConfig flowConfig) {
        TaskSession session = new TaskSession();
        try {
            return runWithConfig(flowConfig, session);
        } catch (Exception e) {
            log.warn(e.getMessage());
            return false;
        }
    }

    boolean runWithConfig(FlowConfig flowConfig, TaskSession taskSession) {
        if (!configCheck(flowConfig)) {
            log.warn("config check error");
            return false;
        }

        var runGraph = buildGraph(flowConfig, taskSession);
        if (Graphs.hasCycle(runGraph)) {
            log.warn("has Cycle");
            return false;
        }
        List<TaskWrapper> runList = new ArrayList<>(runGraph.nodes());
        List<TaskWrapper> readyList = new ArrayList<>(runGraph.nodes());
        while (!runList.isEmpty()) {
            var it = runList.iterator();
            while (it.hasNext()) {
                var task = it.next();
                if (!runGraph.predecessors(task).isEmpty()) {
                    continue;
                }
                it.remove();
                runGraph.removeNode(task);
                if (checkDepTask(task)) {
                    runList.clear();;
                    log.warn("task error: {}", task.getCallable().getClass().getName());
                    break;
                }
                var depFuture = task.getDeps().stream()
                        .map(TaskWrapper::getFuture)
                        .filter(future->!future.isDone())
                        .collect(Collectors.toList());
                if (depFuture.isEmpty()) {
                    task.future = CompletableFuture.supplyAsync(task.callable, executorService);
                } else {
                    task.future = CompletableFuture.allOf(depFuture.toArray(new CompletableFuture[depFuture.size()]))
                            .thenApplyAsync(unused -> depFuture.stream().allMatch(CompletableFuture::join), executorService)
                            .thenApplyAsync(aBoolean -> aBoolean && task.run(), executorService);
                }
            }
        }
        return readyList.stream()
                .map(TaskWrapper::getFuture)
                .filter(Objects::nonNull)
                .allMatch(CompletableFuture::join);
    }

    MutableGraph<TaskWrapper> buildGraph(FlowConfig flowConfig, TaskSession taskSession) {
        var taskMap = this.getTaskMap();
        Map<String, TaskWrapper> runMap = new HashMap<>();
        MutableGraph<TaskWrapper> runGraph = GraphBuilder.directed().allowsSelfLoops(false).build();

        flowConfig.forEach(taskconfig -> {
            log.info("add task {}", taskconfig.task);
            var wrapper = new TaskWrapper(taskMap.get(taskconfig.getTask()));
            wrapper.getCallable().bindSession(taskSession);
            runMap.put(taskconfig.getTask(), wrapper);
            runGraph.addNode(wrapper);
        });
        flowConfig.forEach(taskConfig -> {
            var node = runMap.get(taskConfig.getTask());
            taskConfig.dependencies.forEach(subtask -> {
                var subnode = runMap.get(subtask);
                runGraph.putEdge(node, subnode);
            });
            log.info("task {} add dep {}", taskConfig.getTask(), String.join(",", taskConfig.getDependencies()));
        });
        for (var task : runGraph.nodes()) {
            task.deps = new ArrayList<>(runGraph.predecessors(task));
        }
        return runGraph;
    }

    boolean configCheck(FlowConfig flowConfig) {
        for (var taskConfig : flowConfig) {
            if (!taskSet.contains(taskConfig.getTask())) {
                log.warn("miss {}", taskConfig.getTask());
                return false;
            }
            for (var dep : taskConfig.getDependencies()) {
                if (!taskSet.contains(dep)) {
                    log.warn("{} miss dep {}", taskConfig.getTask(), dep);
                    return false;
                }
            }
        }
        return true;
    }

    boolean checkDepTask(TaskWrapper wrapper) {
        return wrapper.deps.stream()
                .map(TaskWrapper::getFuture)
                .anyMatch(future->future.isDone() && !future.getNow(false));
    }

    Map<String, TaskNode> getTaskMap() {
        return applicationContext.getBeansOfType(TaskNode.class)
                .entrySet().stream()
                .collect(Collectors.toMap(
                        entry -> entry.getValue().getClass().getName().split("\\$\\$")[0],
                        Map.Entry::getValue
                ));
    }

}
