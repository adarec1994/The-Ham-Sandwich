using System;
using System.Collections;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Threading;

namespace ProjectWS.Engine.TaskManager
{
    public class TThread
    {
        public bool isRunning;
        public Thread thread;
        public ConcurrentQueue<Task> tasks = new ConcurrentQueue<Task>();
        public int sleep;
        string name;

        public TThread(string name)
        {
            this.isRunning = false;
            this.name = name;
        }

        public void Enqueue(Task task)
        {
            this.tasks.Enqueue(task);
        }

        public void Boot(int sleep)
        {
            this.sleep = sleep;
            if (!this.isRunning)
            {
                this.isRunning = true;

                // Boot the thread //
                this.thread = new Thread(() => Run())
                {
                    Name = "[Thread]" + this.name,
                    IsBackground = true,
                    Priority = System.Threading.ThreadPriority.AboveNormal
                };
                this.thread.Start();
            }
        }

        void Run()
        {
            while (this.isRunning)
            {
                if(this.tasks.TryDequeue(out Task? task))
                {
                    if (task != null)
                        task.PerformTask();
                }

                if (this.sleep > 0)
                {
                    if (this.tasks.Count == 0)
                    {
                        //Thread.Sleep(this.sleep);

                        try
                        {
                            //Console.WriteLine("newThread going to sleep.");

                            // When newThread goes to sleep, it is immediately 
                            // woken up by a ThreadInterruptedException.
                            Thread.Sleep(this.sleep);
                        }
                        catch (ThreadInterruptedException e)
                        {
                            Console.WriteLine("newThread cannot go to sleep - " +
                                "interrupted by main thread.");
                        }
                    }
                }
            }
        }

        public void Clear()
        {
            this.isRunning = false;
            /*
            if (this.thread != null)
            {
                if (this.thread.IsAlive)
                {
                    this.thread.Interrupt();
                }
            }
            */
        }
    }
}