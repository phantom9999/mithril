#include <iostream>
#include <type_traits>
#include "graph.h"
#include "graph_executor.h"

int main() {
  engine::GraphBuilder graph_builder;
  auto task1 = graph_builder.Add([](){
    std::cout << "task 1\n";
  }).Name("task1");
  auto task2 = graph_builder.Add([](){
    std::cout << "task 2\n";
  }).Name("task2");
  auto task3 = graph_builder.Add([](){
    std::cout << "task 3\n";
  }).Name("task3");
  auto task4 = graph_builder.Add([](){
    std::cout << "task 4\n";
  }).Name("task4");
  auto task5 = graph_builder.Add([](){
    std::cout << "task 5\n";
  }).Name("task5");
  auto task6 = graph_builder.Add([](){
    std::cout << "task 6\n";
  }).Name("task6");
  auto task7 = graph_builder.Add([](){
    std::cout << "task 7\n";
  }).Name("task7");
  auto task8 = graph_builder.Add([](){
    std::cout << "task 8\n";
  }).Name("task8");
  task1.Precede(task3, task4);
  task2.Precede(task5);
  task3.Precede(task7);
  task4.Precede(task6);
  task5.Precede(task6);
  task6.Precede(task7, task8);

  auto graph = graph_builder.Freeze();
  engine::GraphExecutor executor;
  auto session = executor.BuildNewSession(graph);
  session->Run();
}

