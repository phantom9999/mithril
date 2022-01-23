package com.example.taskflow_java.taskflow;

import lombok.extern.log4j.Log4j2;

import java.util.function.Supplier;

@Log4j2
public abstract class TaskNode implements Supplier<Boolean> {
    protected TaskSession taskSession;

    void bindSession(TaskSession taskSession) {
        this.taskSession = taskSession;
    }

    @Override
    public Boolean get() {
        if (taskSession == null) {
            return false;
        }
        long beginTime = System.currentTimeMillis();
        boolean result = false;
        try {
            result = run();
        } catch (Exception e) {
            log.warn("{} catch exception of {}", this.getClass().getName(), e.getMessage());
            return false;
        }

        long cost = System.currentTimeMillis() - beginTime;
        if (!result) {
            log.warn("{} run fail, cost {}ms", this.getClass().getName(), cost);
        } else {
            log.info("{} run success, cost {}ms", this.getClass().getName(), cost);
        }
        return result;
    }

    protected abstract boolean run();
}
