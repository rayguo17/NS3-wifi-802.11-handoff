BEGIN{
    init =0;
    cnt=0;
    FS = "[() \t]";
    #length=524;
    send_cnt=0;
    rev_cnt=0;
}
{
    action = $1;
    time = $2;
    flag=0;
    seq=0;
    ack =0;
    flag_payload=false;
    for (i=1;i<=NF;i++)
    {
        if($i ~ /ns3::TcpHeader/){
            flag=1;
            #print($i);
        }
        if($i ~ /Payload/)
        {
            #print($i);
            flag_payload =1;
        }
        if($i ~ /Seq=/)
        {
            #myPacketID = $(i+1);
            split($i,b,"=");
            #print(b[2]);
            seq = b[2];

        }
        if($i ~ /Ack=/)
        {
            split($i,c,"=");
            ack = c[2];
        }

        
    }
    if(flag!=0 &&  seq>=1){
        if(action == "-" && flag_payload==1)
        {
            send_cnt = (seq-1)/524;
            #print();
            if(start_time[send_cnt]==0)
            {
                #print("haha");
                start_time[send_cnt] = time;
            }
        }
        if(action =="r")
        {
            ack_cnt = (ack-1)/524-1;
            #print(ack_cnt);
            #print(time);
            if(end_time[ack_cnt]==0)
            {
                
                end_time[ack_cnt]=time;
            }
            else {
                #print("haha");
            }

        }

    }
    
    
}
END{
    printf("%s\t%s\n",start_time[0],0);
    for(j=1;j<ack_cnt;j++){
        if(end_time[j]==0)continue;
        throughput = (j*524/(end_time[j]-start_time[0]))*8/1000;
        printf("%s\t%s\n",end_time[j],throughput);
    }
    printf("%.2f\t%.2f\n",end_time[j-1],0);
}