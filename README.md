# 加入链路可用带宽探测机制

## 当前已有修改

### TcpSocketBase中
sendDataPacket函数中添加代码段
      
    /**
     * \brief check whether need to update the segmentsize per RTT
    */
    if (Simulator::Now().GetSeconds() - m_tcb->m_lastUpdate.GetSeconds() >= m_lastRtt.Get().GetSeconds())
    {
      // std::cout << "update Segmentsize" << m_lastRtt.Get().GetSeconds() << std::endl;
      MtuDecision mtuDecision;
      // packet 中携带信息做决策
      PriorityTag tag;
      bool peekTag = p->PeekPacketTag(tag);
      if (peekTag)
      {
        uint32_t flow_size = tag.GetFlowsizeTag();
        double RTT = m_lastRtt.Get().GetSeconds() * 1000;
        // double RTT = m_tcb->m_minRtt.GetMicroSeconds();
        // cout the rtt , check whether it is 0
        // std::cout << RTT << std::endl;
        int bandwidth = int(tag.GetBandwidth());
        int best_mtu = mtuDecision.FindBestMtu(flow_size, bandwidth, RTT, 0, 0);
        // std::cout << "after update " << best_mtu << std::endl;
        m_tcb->m_segmentSize = best_mtu - 40;
        m_tcb->m_lastUpdate = Simulator::Now();
      }
    }

EstimateRtt中添加函数段

   /**
       * addtion part begin
      */
      // if this is the ack packet for the first packet

      if (m_tcb->m_avgRtt == 0)
      {
        m_tcb->m_avgRtt = double(m_lastRtt.Get().GetMicroSeconds()) / 1000;
        // std::cout << m_tcb->m_avgRtt << std::endl;
      }
      else
      {
        m_tcb->m_avgRtt = 0.5 * m_tcb->m_avgRtt + (0.5 * m_lastRtt.Get().GetMicroSeconds()) / 1000;
      }

      m_tcb->m_currentRtt = double(m_lastRtt.Get().GetMicroSeconds()) / 1000;
      // std::cout << m_tcb->m_currentRtt << std::endl;
      // std::cout << "update the rtt" << std::endl;
      /**
       * addtion part ends
      */

做决策需要的信息 
    
    1.Bandwidth --initial extend from scratch, then estimated from data delivered in the past time slot(6-10RTT)
    2.剩余流大小 --remaining flow size
    3.RTT --根据过去一段时间内收到的包动态估计
    4.发送时延 --根据buffer长度动态估计---> 改为初始根据buffer长度，之后根据RTT
    5.接收端时延 --暂不考虑

需要在TCPSocketBase中维护的数据结构
    
    1.上次更新时间（初始化为0）
    2.上次更新时确认的字节数 （初始化为0）
    //暂不需要
    3.当前流所占带宽（初始化时从scratch中读出来）
    
    //已有的
    4.RTT History
    5.RTT Estimated

    第一次包大小的计算在生成application的时候确定
    需要从scratch中读出更新间隔（int值 6-8）
    需要从scratch中读出来网卡发送队列预计的平均长度 
        可以在scratch中定义，在tcpsocketbase和netdevice中extern
    
### 2020-12-16日修改

  1.重新设定初始CWND，修改了之前只计算端侧传输时延的BUG
  2.在初始估计时，发送时延使用网卡中的队列进行估计，在流运行过程中直接使用RTT进行估计
  3.修改了TimeStamp的精度（之前的时延显示都是0和1），设置为微秒，整个运行过程中时间统一为毫秒
  4.findbestMTU函数中输入的传输时延和处理时延的初始值修改为double
  5.为了便于增长，mtu设为1540，3040，4540，9040

  仍然存在的问题：
    RTT出现了小于传播时延的问题