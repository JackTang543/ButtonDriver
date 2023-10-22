#include "stm32f1xx_hal.h"

//SightseerGeneralButtonDriver SigGenBtnDrv
//2023.10.22 v1.0


//本模块自定义的返回值
typedef enum{
    sGBD_RET_OK = 0,
    sGBD_RET_ERR
}sGBD_RET_t;

//按键电平状态
typedef enum{
    sGBD_LEVEL_LOW = 0,
    sGBD_LEVEL_HIGH
}sGBD_LEVEL_t;

//按键长按时该如何触发回调
typedef enum{
    sGBD_LP_SGL_TRIG = 0,    //长按时只触发一次
    sGBD_LP_LOP_TRIG = 1     //长按循环触发
}sGBD_LP_FUNC_t;

//按键被触发的类型(用户按键输入的操作)
typedef enum{
    sGBD_TRIG_IDLE = 0,  //触发为空,也就是没有触发
    sGBD_TRIG_PRES,      //单击
    sGBD_TRIG_LPRS,      //长按
    sGBD_TRIG_2PRS       //双击
}sGBD_TRIG_t;

//按键电平变化标志类型
typedef enum{
    sGBD_LEVEL_RISING = 0,
    sGBD_LEVEL_FALING,
    sGBD_LEVEL_NOCHEG,
    sGBD_LEVEL_SKIP        //非操作,就是状态机有两个条件,遇到这个条件就忽略继续
}sGBD_LEVEL_CHGE_t;

//按键状态机的状态
typedef enum{
    sGBD_FSM_S_IDLE = 0,    //状态机状态之空闲
    sGBD_FSM_S_PRES,        //状态机状态之按下
    sGBD_FSM_S_WT2P,        //等待第二次按下
    sGBD_FSM_S_RLSD         //松手
}sGBD_FSM_STATE_t;

//按键结构体
typedef struct{
    sGBD_LEVEL_CHGE_t m_level_change;       //按键电平变化标志
    sGBD_LEVEL_t      m_btn_l_curr_status;  //按键电平当前状态
    sGBD_LEVEL_t      m_btn_l_prev_status;  //按键电平上一次状态
    sGBD_FSM_STATE_t  m_btn_fsm_state;      //按键状态机的现态
}sGBD_BUTTON_t;

//按键状态机某个状态要做的事 StateOperationCheck
typedef enum{
    SOC_00 = 0, //这个SOC在函数里总是返回OK
    SOC_01,
    SOC_02,
    SOC_03,
    SOC_04,
    SOC_05,
    SOC_06,
    SOC_CLR     //用来清空时间变量
}sGBD_FSM_OPERATION_t;

//用于状态机的状态转移表
typedef struct{
    sGBD_FSM_STATE_t     m_prev_state;//上态
    sGBD_FSM_STATE_t     m_tran_state;//次态
    sGBD_LEVEL_CHGE_t    m_tran_flag; //切换条件
    sGBD_FSM_OPERATION_t m_soc;       //状态操作检查,相当于第二个切换条件
    sGBD_TRIG_t          m_trig;      //这个状态转换完后要执行什么操作吗
}sGBD_FSM_TRANSFORM_t;

//用来给用户注册回调,以便于按键有触发时通知你
typedef void (*BtnTrigCb_t)(uint16_t key_id,sGBD_TRIG_t key_event);
//用于让用户传入一个滴答定时器函数,增强通用性
typedef uint32_t (*TickerFuncCb_t)(void);


class sGBD{
public:

    //按键id
    uint16_t key_id;
    //你想多久调用我的处理函数一次呢,我不知道,需要你告诉我,不然就罢工
    uint16_t handler_cycle_ms;
    //按键长按模式,有单次触发和循环触发,用于需要长按数值持续增加的场景
    sGBD_LP_FUNC_t key_lp_func;
    //按键长按循环调用的时间间隔,单位ms
    uint16_t key_lp_cycle_time_ms;

    sGBD(uint16_t create_key_id,sGBD_LP_FUNC_t lpress_func,sGBD_LEVEL_t idle_level,uint16_t handler_cycle);
    ~sGBD();
    sGBD_RET_t sGBD_Button_handler(uint8_t key_state);
    sGBD_RET_t sGBD_SetButtonLPressLoopCycle(uint16_t cycle_time_ms);
    sGBD_RET_t sGBD_SetButtonTrigCb(BtnTrigCb_t trig_cb);
    sGBD_RET_t sGBD_SetButtonLP_2P_Time(uint16_t time_lp_ms, uint16_t time_2p_ms);

    static sGBD_RET_t sGBD_SetTickerFunc(TickerFuncCb_t userTick);

private:

    //按键触发标志位
    sGBD_TRIG_t btn_trig_flag;
    //用于保存用户传入的触发回调函数
    BtnTrigCb_t btn_trig_callback;
    //按键电平状态保存
    sGBD_LEVEL_t btn_level;
    //按键空闲时电平
    sGBD_LEVEL_t idle_level;
    //按键长按界定时间,单位ms
    uint16_t btn_lpress_time;
    //按键双击界定时间,单位ms
    uint16_t btn_2press_time;
    //按键各种标志位参数的结构体
    sGBD_BUTTON_t button;
    //状态转换规则表
    static sGBD_FSM_TRANSFORM_t fsm_ruler[7];

    void sGBD_EventTrig(uint16_t key_id,sGBD_TRIG_t key_event);
    void Button_CompareStatus(void);
    sGBD_RET_t Button_FSM_Handler(void);
    sGBD_RET_t FSM_StateOperationCheck(sGBD_FSM_OPERATION_t soc);
    uint32_t Btn_GetTick(void);
};

