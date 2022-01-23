package com.example.taskflow_java.taskflow;

import java.util.Map;
import java.util.Optional;
import java.util.concurrent.ConcurrentHashMap;

/**
 * .
 */
public class TaskSession {
    private Map<String, Object> objectMap = new ConcurrentHashMap<>();

    /**
     * 获取session
     * @param nameClass
     * @param <TypeName>
     * @return
     */
    public <TypeName>Optional<TypeName> get(Class<TypeName> nameClass) {
        return Optional.ofNullable(objectMap.get(nameClass.toString()))
                .filter(o -> o.getClass() == nameClass)
                .map(object->(TypeName) object);
    }

    /**
     * 将数据写入session
     * @param object
     */
    public void put(Object object) {
        objectMap.put(object.getClass().toString(), object);
    }
}
