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

做决策需要的信息 
    
    1.Bandwidth --initial extend from scratch, then estimated from data delivered in the past time slot(6-10RTT)
    2.剩余流大小 --remaining flow size
    3.RTT --根据过去一段时间内收到的包动态估计
    4.发送时延 --根据buffer长度动态估计
    5.接收端时延 --暂不考虑

需要在TCPSocketBase中维护的数据结构
    
    1.上次更新时间（初始化为0）
    2.上次更新时确认的字节数 （初始化为0）
    3.当前流所占带宽（初始化时从scratch中读出来）
    4.RTT History
    5.RTT Estimated

    第一次包大小的计算在生成application的时候确定
    需要从scratch中读出更新间隔（int值 6-8）
    需要从scratch中读出来网卡发送队列预计的平均长度 
        可以在scrstch中定义，在tcpsocketbase和netdevice中extern
    
