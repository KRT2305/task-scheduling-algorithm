#include <vector>
#include <algorithm>
#include <unistd.h>
#include <array>

#include "Task.cc"
//#include "ExecutionUnit.cc"

using namespace std;

struct CloudTask{

  int t_send;
  int t_c_exec;
  int t_recv;
  
};


std::vector<ExecutionUnit> get_execution_units(int core_count){

  std::vector<ExecutionUnit> cpus;

  /**
   * One unit for every CPU core
   * Order the processor in descending order of power
   * - this might be important
   */
  for(int i=0; i<core_count; i++){
    ExecutionUnit eu(i+1, 'l');
    cpus.push_back(eu);
  }

  /**
   * 1 Execution Unit for the cloud
   */
  ExecutionUnit eu(core_count+1, 'c');
  cpus.push_back(eu);

  return cpus;
  
}


bool compare_by_priority(Task t1, Task t2)
{

  /**
   *  Comparator for sorting the tasks by priority
   */
  return t1.get_priority() > t2.get_priority();
   
}


void sort_by_priority(std::vector<Task> &tasks)
{

  /**
   *  Sorting tasks by priority in descending order
   */
  std::sort (tasks.begin(), tasks.end(), compare_by_priority);
  
}


std::vector<Task> construct_tasks(int **graph,
				  int job_count,
				  std::vector<int> exit_tasks)
{

  /**
   *  Construct the task graph from the adjacency matrix.
   *
   *  Each job becomes an Object of type 'Task'.
   *
   *  It would have pointers to its parent tasks
   *  as well as child tasks for easy traversal.
   *
   */
  std::vector<Task> tasks;

  /**
   * Find the parent(s) of each task
   */
  for(int j=0; j<job_count; j++){
    Task task(j+1);
    for(int i=0; i<j; i++){
      if (graph[i][j] == 1){
	task.add_parent(&tasks[i]);
      }
    }
    tasks.push_back(task);
  }

  /**
   * Enrich the graph with children of each graph
   */
  for(int i=0; i<job_count; i++){
    for(int j=0; j<job_count; j++){
      if(graph[i][j] == 1){
	tasks[i].add_child(&tasks[j]);
      }
    }
  }

  for(int k=0; k<exit_tasks.size(); k++){
    tasks[exit_tasks[k]].set_is_exit(true);
  }
	
  return tasks;
  
}





void calculate_and_set_priority(Task &task){

  /**
   *  Recursive implementation to compute the 
   *  initial priority of tasks
   */

  if(task.get_is_exit()){
    task.set_priority(task.get_cost());
  } else{

    std::vector<Task*> children = task.get_children();
    std::vector<float> child_priorities;

    /*
     * Get the calculated total priority of children
     */
    for(int j=0; j<children.size(); j++){
      calculate_and_set_priority(*children[j]);
      child_priorities.push_back((*children[j]).get_priority());
    }

    float max_child_priority =
      *std::max_element(std::begin(child_priorities), std::end(child_priorities));

    float cost = task.get_cost();
    float priority = cost + max_child_priority;
    task.set_priority(priority);
    
  }

}


void start(Task &task,
  	   ExecutionUnit *cpu,
	   std::array<std::array<int,3>, 10> core_table){
  
  task.set_is_running(true);
  task.set_cpu(cpu);

  int cpu_id = cpu->get_id();
  int task_index = task.get_id() - 1;
  int cpu_index = cpu_id - 1;

  float ticks_to_finish;
  
  if(cpu_id <= 3){
    // local CPU - pick time from core table
    ticks_to_finish = (float)core_table[task_index][cpu_index];
    std::cout<<"\nTicks to finish for task "<<task.get_id()<<" :"<<ticks_to_finish<<"\n";
  } else{
    // hard coding for now
    ticks_to_finish = 5;
    std::cout<<"\nTicks to finish for cloud task "<<task.get_id()<<" :"
	     <<ticks_to_finish<<"\n";
  }
  task.set_ticks_to_finish(ticks_to_finish);

  std::cout<<"\nRunning task "<<task.get_id()<<" on CPU with ID: "<<cpu->get_id()<<"\n";
  
}




ExecutionUnit* get_free_cpu(std::vector<ExecutionUnit> &cpus,
			    Task &task){

  if (task.get_type() == 'c'){

    ExecutionUnit* cloud_cpu = &cpus[cpus.size() - 1];

    if (cloud_cpu->get_available()){
      return cloud_cpu;
    }

    return NULL;
    
  } else {


  
    ExecutionUnit* free_unit = NULL;

    /*
     *  Priority of CPUS
     *  If most powerful local core (CPU 3) is available return it
     *  Else try cloud cpu (CPU 4)
     *  Else try local cpu (CPU 2)
     *  Else try local cpu (CPU 1)
     *
     */
    if(cpus[cpus.size()-2].get_available()){
      free_unit = &cpus[cpus.size()-2];
    } else if (cpus[cpus.size()-1].get_available()){
      free_unit = &cpus[cpus.size()-1];
    } else if (cpus[cpus.size()-3].get_available()){
      free_unit = &cpus[cpus.size()-3];
    } else if (cpus[cpus.size()-4].get_available()){
      free_unit = &cpus[cpus.size()-4];
    } 
    
    return free_unit;
  }
  
}


void assign(std::vector<Task> &ready_queue,
	    std::vector<Task> &running_queue,	    
	    std::vector<ExecutionUnit> &cpus,
	    std::array<std::array<int,3>, 10> core_table){

  /**
   * For each task in the ready queue, look
   * for availability of cores / cloud instance.
   *
   * If some processing unit is available assign it to it.
   * set the ticks to finish for the task
   *
   * If not break out, we cannot assign any more tasks
   * anyway.
   */
  for(int i=0; i<ready_queue.size(); i++){

    Task task = ready_queue[i];
    ExecutionUnit* cpu = get_free_cpu(cpus, task);

    //    std::cout<<"\nAssigning "<<task.get_id()<<" CPU"<<cpu->get_id();

    if(cpu != NULL){
      start(task, cpu, core_table);

      std::cout<<"&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n";
      std::cout<<task.get_id()<<"\n";
      std::cout<<task.get_progress()<<"\n";
      std::cout<<task.get_ticks_to_finish()<<"\n";
      std::cout<<"&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n";      
      

      /**
       * Remove the task from ready queue and assign it to running queue
       */
      running_queue.push_back(task);
      ready_queue.erase(std::remove(ready_queue.begin(),
				  ready_queue.end(), task),
		      ready_queue.end());
    }else{
      std::cout<<"No free execution units available. Scheduler will wait until the next tick!\n";
      break;
    }
    
  }
  
}


void print_ready_tasks(std::vector<Task> &ready_queue){

  std::cout<<"Tasks in ready queue: ";
  for(int i=0; i<ready_queue.size(); i++){
    std::cout<<ready_queue[i].get_id()<<"\t";
  }
  std::cout<<"\n";
}


void stop_execution(Task &task,
		    std::vector<Task> &running_queue){

  task.set_is_running(false);
  task.set_is_finished(true);

  std::cout<<"Finished execution of task: "<<task.get_id()<<"\n";
      
  // Free the CPU
  ExecutionUnit* cpu = task.get_cpu();
  cpu->set_available(true);

  // remove the task from the running queue - wouldn't this
  // be a ConcurrentExecutionException ?? -
  // If there is, keep indices to remove together
  // and do an iteration at the end
  // Need to check later - Sreejith
  running_queue.erase(std::remove(running_queue.begin(),
				  running_queue.end(), task),
		      running_queue.end());
    
}

void remove_finished_tasks(std::vector<Task> &running_queue){

  for(int i=0; i<running_queue.size(); i++){
    
    Task task = running_queue[i];
    if(task.get_progress() >= task.get_ticks_to_finish()){

      std::cout<<"Finished------------->>\n";

      /**
       * Ticks have reached the ticks to finish the task
       * Remove the task from running queue
       * Free the CPU
       */
      stop_execution(task, running_queue);
      
    }
    
  }
  
}

void try_unlocking(std::vector<Task> &tasks_in_pool,
		   std::vector<Task> &ready_queue){

  /**
   *  For every task in pool,
   * Check if all parents have finished execution.
   *
   * If they have, add it to ready queue and remove
   * from tasks_in_pool
   */
  for(int i=0; i<tasks_in_pool.size(); i++){

    Task task = tasks_in_pool[i];
    std::vector<Task*> parents =  task.get_parents();
    bool can_start = true;
    
    for(int j=0; j<parents.size(); j++){
      if(!(parents[j]->get_is_finished())){
      	can_start = false;
      	break;
      }
    }

    if(can_start){
      std::cout<<"\n Adding "<<task.get_id()<< " to the ready queue";
      task.set_is_unlocked(true);
      ready_queue.push_back(task);
      tasks_in_pool.erase(std::remove(tasks_in_pool.begin(),
    				      tasks_in_pool.end(), task),
    			  tasks_in_pool.end());
    }
  
  }
  
}


void run(std::vector<Task> &running_queue){

  /**
   * Increment the tick for each task.
   * If the task progress  = ticks to finish
   * set its execution unit availablity to true.
   *
   */
  for(int i=0; i<running_queue.size(); i++){
    Task task = running_queue[i];
    float progress = task.get_progress() + 1.0;
    std::cout<<"Progress outside-->"<<task.get_id()<<"=="<<progress<<"\n";
    task.set_progress(progress);
  }
  
}


void run_scheduler(std::vector<Task> &tasks_in_pool,
		   std::vector<Task> &ready_queue,
		   std::vector<ExecutionUnit> &cpus,
		   std::array<std::array<int,3>, 10> core_table){

  std::vector<Task> running_queue;
  
  /**
   *  Every time it goes into this loop,
   *  it is one tick (shown in Fig. 3)
   */
  int i=0;
  do{

    /**
     * If any job has finished, free the CPU
     * on which it has been running
     */
    remove_finished_tasks(running_queue);
    try_unlocking(tasks_in_pool, ready_queue);
    
    if (ready_queue.size() > 0){

    // sort the ready queue by priority
      assign(ready_queue,
    	     running_queue,
    	     cpus,
	     core_table);

    }

    std::cout<<"\nTick: "<<i++;
    // increment the running tick for every running task
    std::cout<<"\nPool queue size:"<<tasks_in_pool.size()<<"\n";
    std::cout<<"\nRunning queue size:"<<running_queue.size()<<"\n";
    std::cout<<"\nReady queue size:"<<ready_queue.size()<<"\n";      
    run(running_queue);
    sleep(1);
  }while(tasks_in_pool.size() > 0);

}
