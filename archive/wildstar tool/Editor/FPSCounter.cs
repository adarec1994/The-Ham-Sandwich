using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ProjectWS.Editor
{
    public class FPSCounter
    {
        float m_secondCounter;
        //These are not the real fps, just temporary
        float m_tempFps;
        //This float is the fps we should use
        float fps;

        public void Update(float deltaTime)
        {
            /* How ever you get deltaTime in seconds */

            if (m_secondCounter <= 1)
            {
                m_secondCounter += deltaTime;
                m_tempFps++;
            }
            else
            {
                //"fps" are the actual fps
                fps = m_tempFps;
                m_secondCounter = 0;
                m_tempFps = 0;
            }
        }

        public float Get()
        {
            return fps;
        }

        public string GetString()
        {
            //Do something with the fps
            return $"FPS: {fps}";
        }
    }
}
