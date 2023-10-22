#include "sig_btn_driver.h"

//c++17
//基于有限状态机的按键驱动器
//基于cpp with class编程思想

//所有对象的滴答定时器都是一个
static TickerFuncCb_t BtnDrvTickFunc;
//状态机转换表
sGBD_FSM_TRANSFORM_t sGBD::fsm_ruler[7] = {
        //从sGBD_FSM_S_IDLE转换到sGBD_FSM_S_PRES需要条件sGBD_LEVEL_FALING和SOC_01,这个操作不触发动作,下同
        {sGBD_FSM_S_IDLE, sGBD_FSM_S_PRES, sGBD_LEVEL_FALING, SOC_01, sGBD_TRIG_IDLE},
        {sGBD_FSM_S_PRES, sGBD_FSM_S_WT2P, sGBD_LEVEL_RISING, SOC_03, sGBD_TRIG_IDLE},
        {sGBD_FSM_S_PRES, sGBD_FSM_S_PRES, sGBD_LEVEL_NOCHEG, SOC_06, sGBD_TRIG_IDLE},
        {sGBD_FSM_S_PRES, sGBD_FSM_S_IDLE, sGBD_LEVEL_SKIP  , SOC_02, sGBD_TRIG_LPRS},
        {sGBD_FSM_S_WT2P, sGBD_FSM_S_RLSD, sGBD_LEVEL_SKIP  , SOC_04, sGBD_TRIG_IDLE},
        {sGBD_FSM_S_RLSD, sGBD_FSM_S_IDLE, sGBD_LEVEL_SKIP  , SOC_00, sGBD_TRIG_PRES},
        {sGBD_FSM_S_WT2P, sGBD_FSM_S_IDLE, sGBD_LEVEL_FALING, SOC_05, sGBD_TRIG_2PRS}
    };

/*@brief  GeneralButtonDriver的构造函数
*         
* @param  uint16_t:你要创建按键的id号
* @param  sGBD_LP_FUNC_t:长按时该如何动作
* @param  sGBD_LEVEL_t:空闲电平
* @param  uint16_t:你要多久调用一次我的处理函数
*
* @return 无
*/
sGBD::sGBD(uint16_t create_key_id,sGBD_LP_FUNC_t lpress_func,sGBD_LEVEL_t idle_level,uint16_t handler_cycle){
    //把用户传入的按键id号赋值给这个对象的id号上去
    this->key_id = create_key_id;
    //设置这个对象的按键长按模式
    this->key_lp_func = lpress_func;
    //设置按键空闲时的电平
    this->idle_level = idle_level;

    //默认长按,双击的界定时间
    this->btn_lpress_time = 500;
    this->btn_2press_time = 150;

    //按键初始化
    button.m_level_change      = sGBD_LEVEL_NOCHEG;
    button.m_btn_l_curr_status = sGBD_LEVEL_HIGH;
    button.m_btn_l_prev_status = sGBD_LEVEL_HIGH;
    button.m_btn_fsm_state     = sGBD_FSM_S_IDLE;

    handler_cycle_ms = handler_cycle;
}
/*@brief  GeneralButtonDriver的析构函数
*         暂时用不到
*         
* @param  无
*
* @return 无
*/
sGBD::~sGBD(){

}
/*@brief  FSM_StateOperationCheck()状态操作检查
*         用于状态机的第二个操作
*
* @param  sGBD_FSM_OPERATION_t:状态表内的操作调用
*
* @return sGBD_RET_t:有些操作需要返回sGBD_RET_ERR,有些则是sGBD_RET_OK
*/
sGBD_RET_t sGBD::FSM_StateOperationCheck(sGBD_FSM_OPERATION_t soc){
    //注意,这个函数是根据状态转换示意图写出来的,直接看可能比较难看懂
    //用于保存一些时间变量
    static uint32_t time_1;
    static uint32_t time_2;
    static uint32_t time_3;
    static uint32_t time_4;
    //做完当前需要做的操作,然后再返回ERR或者OK
    switch (soc) 
    {
    case SOC_00:

        //这个SOC总是返回OK
        return sGBD_RET_OK;

        break;
    case SOC_01:

        //第一次的时候会记录一次时间
        if(time_1 == 0){
            time_1 = Btn_GetTick();
        }
        //返回OK
        return sGBD_RET_OK;

        break;
    case SOC_02:
        
        //长按检查
        if(time_2 >= btn_lpress_time){
            return sGBD_RET_OK;
        }else{
            return sGBD_RET_ERR;
        }

        break;
    case SOC_03:

        //第一次的时候会记录一次时间
        if(time_3 == 0){
            time_3 = Btn_GetTick();
        }
        return sGBD_RET_OK;

        break;
    case SOC_04:
        
        time_4 += handler_cycle_ms;
        if(time_4 >= btn_2press_time){
            return sGBD_RET_OK;
        }else{
            return sGBD_RET_ERR;
        }

        break;
    case SOC_05:
        
        if(time_4 >= 30){
            return sGBD_RET_OK;
        }else{
            return sGBD_RET_ERR;
        }

        break;
    case SOC_06:
        
        time_2 += handler_cycle_ms;
        return sGBD_RET_OK;

        break;
    case SOC_CLR:
      
        //清空变量,以免影响下一次的操作
        time_1 = 0;
        time_2 = 0;
        time_3 = 0;
        time_4 = 0;
        return sGBD_RET_OK;
    
    break;
    
    default:
        //程序运行到这里肯定是出错啦
        return sGBD_RET_ERR;
        break;
    }
}
/*@brief  sGBD_SetButtonLPressLoopCycle()
*         设置当按键处于长按循环触发时,每次触发时的时间,单位ms,其他模式无效
*
* @param  uint16_t:你想你的按键被用户长按时,多长时间调用一次你的回调
*
* @return sGBD_RET_t:用户输入0则会返回sGBD_RET_ERR
*/
sGBD_RET_t sGBD::sGBD_SetButtonLPressLoopCycle(uint16_t cycle_time_ms){
    if(cycle_time_ms != 0){
        this->key_lp_cycle_time_ms = cycle_time_ms;
        return sGBD_RET_OK;
    }else{
        //用户输入错误
        return sGBD_RET_ERR;
    }
}
/*@brief  sGBD_SetButtonTrigCb()
*         由用户设置按键的触发回调,按键有按下就会通知用户
*
* @param  BtnTrigCb_t:回调函数指针,具体请查阅.h文件
*
* @return sGBD_RET_t:用户传入空指针则会返回sGBD_RET_ERR
*/
sGBD_RET_t sGBD::sGBD_SetButtonTrigCb(BtnTrigCb_t trig_cb){
    if(trig_cb != nullptr){
        //指针合法,注册回调!
        this->btn_trig_callback = trig_cb; 
        return sGBD_RET_OK;
    }else{
        //cnm,想跑飞了是吧
        return sGBD_RET_ERR;
    }
}
/*@brief  sGBD_EventTrig()
*         有按键触发事件发生时,状态机会调用此函数,此函数再调用用户回调
*
* @param  uint16_t:发生触发事件的按键的id号
* @param  sGBD_TRIG_t:按键发生的事件
*
* @return 无
*/
void sGBD::sGBD_EventTrig(uint16_t key_id,sGBD_TRIG_t key_event){
    switch(key_event){
        case sGBD_TRIG_PRES:
            //叫用户处理按下操作
            btn_trig_callback(key_id,sGBD_TRIG_PRES);
            //用户处理完毕,清空标志位
            btn_trig_flag = sGBD_TRIG_IDLE;
        break;
        case sGBD_TRIG_LPRS:
            //叫用户处理长按操作
            btn_trig_callback(key_id,sGBD_TRIG_LPRS);
            //用户处理完毕,清空标志位
            btn_trig_flag = sGBD_TRIG_IDLE;
        break;
        case sGBD_TRIG_2PRS:
            //叫用户处理双击操作
            btn_trig_callback(key_id,sGBD_TRIG_2PRS);
            //用户处理完毕,清空标志位
            btn_trig_flag = sGBD_TRIG_IDLE;
        break;
        default:
            //如果代码运行到这里肯定是出错了
        break;
    }
}
/*@brief  sGBD_SetButtonLP_2P_Time()
*         用户设置长按和双击的界定时间,你不设置会有一个默认的值
*
* @param  uint16_t:长按界定时间,单位ms
* @param  uint16_t:双击界定时间,单位ms
*
* @return sGBD_RET_t:返回ERR说明你传入的数据太离谱了,会影响响应速度
*/
sGBD_RET_t sGBD::sGBD_SetButtonLP_2P_Time(uint16_t time_lp_ms, uint16_t time_2p_ms){
    this->btn_lpress_time = time_lp_ms;
    this->btn_2press_time = time_2p_ms;
    return sGBD_RET_OK;
}
/*@brief  Button_CompareStatus()
*         实现按键比较状态,更新状态,此函数用于检查按键的边沿,由于定时调用,不需要消抖
*
* @param  无
*
* @return 无
*/
void sGBD::Button_CompareStatus(){
    //上一次的状态等于这一次的状态
    this->button.m_btn_l_prev_status = this->button.m_btn_l_curr_status;
    //这一次的状态由读取到的电平决定
    this->button.m_btn_l_curr_status = this->btn_level;
    
    //下面对两个状态进行处理得到上升/下降沿
    if(this->button.m_btn_l_curr_status == this->button.m_btn_l_prev_status){
        //如果当前电平等于上次,说明按键电平没有变化
        this->button.m_level_change = sGBD_LEVEL_NOCHEG;
    }
    else if((this->button.m_btn_l_prev_status == sGBD_LEVEL_HIGH) && (this->button.m_btn_l_curr_status == sGBD_LEVEL_LOW)){
        //如果上一次是高,现在是低,则说明有下降沿
        this->button.m_level_change = sGBD_LEVEL_FALING;
    }
    else if((this->button.m_btn_l_prev_status == sGBD_LEVEL_LOW) && (this->button.m_btn_l_curr_status == sGBD_LEVEL_HIGH)){
        //如果上一次是低,现在是高,则说明有上升沿
        this->button.m_level_change = sGBD_LEVEL_RISING;
    }
    //我们现在得到了按键电平的上升/下降沿或者无变化标志位
}
/*@brief  Button_FSM_Handler()
*         这是这个文件的核心代码,遍历整个状态转移表来给出操作
*
* @param  无
*
* @return sGBD_RET_t:总是返回OK
*/
sGBD_RET_t sGBD::Button_FSM_Handler(){
    //遍历状态表
    for(uint8_t ruler_num = 0; ruler_num < (sizeof(fsm_ruler) / sizeof(fsm_ruler[0])); ruler_num++){
        //进行状态切换,必须满足3个条件:
        //1.按键当前状态必须和规则表中上态相同
        //2.按键电平变化标志位必须和规则表中相同
        //3.进行状态操作检查SOC时,返回值必须为sGBD_RET_OK
        if((button.m_btn_fsm_state == fsm_ruler[ruler_num].m_prev_state)    && 
            ((button.m_level_change  == fsm_ruler[ruler_num].m_tran_flag)   ||
            fsm_ruler[ruler_num].m_tran_flag == sGBD_LEVEL_SKIP)            &&
            FSM_StateOperationCheck(fsm_ruler[ruler_num].m_soc) == sGBD_RET_OK ){
                //开始进行状态切换
                //切换到次态
                button.m_btn_fsm_state = fsm_ruler[ruler_num].m_tran_state;
                //如果需要触发动作,就触发对应的动作
                if(fsm_ruler[ruler_num].m_trig != sGBD_TRIG_IDLE){
                    //先清空time标志
                    FSM_StateOperationCheck(SOC_CLR);
                    //然后触发动作
                    sGBD_EventTrig(this->key_id,fsm_ruler[ruler_num].m_trig);
                }
                //动作触发完成
            }
    }
    return sGBD_RET_OK;
}
/*@brief  sGBD_Button_handler()
*         由用户调用的处理句柄,必须定时调用,以驱动整个模块
*
* @param  uint8_t:需要你用你平台上的按键读取函数读取到按键的电平值传进来,非0既真
*
* @return sGBD_RET_t:总是返回OK
*/
sGBD_RET_t sGBD::sGBD_Button_handler(uint8_t key_state){
    //任何非0的数AND上1都得1,相当于取出最低位,再转换成枚举类型
    this->btn_level = (sGBD_LEVEL_t)(key_state & 0b00000001);
    //进行电平比较得到边沿
    Button_CompareStatus();
    //给状态机处理
    Button_FSM_Handler();

    return sGBD_RET_OK;
}
/*@brief  Btn_GetTick()
*         本文件用到的获取时间戳的函数,本质是用户传入的滴答函数
*
* @param  无
*
* @return uint32_t:由用户层得到的时间戳
*/
uint32_t sGBD::Btn_GetTick(void){
    return BtnDrvTickFunc();
}
/*@brief  sGBD_SetTickerFunc()
*         由用户传入一个滴答定时器,用来增强通用性
*
* @param  TickerFuncCb_t:用户的滴答定时器函数
*
* @return sGBD_RET_t:如果传入空指针则返回ERR
*/
sGBD_RET_t sGBD::sGBD_SetTickerFunc(TickerFuncCb_t userTick){
    if(userTick != nullptr){
        BtnDrvTickFunc = userTick;
        return sGBD_RET_OK;
    }else{
        return sGBD_RET_ERR;
    }
}
