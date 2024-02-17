# General Button Driver v2
## 通用按键驱动v2

### 功能
用于驱动按键，获取按键触发事件。

### 特点
1. 类linux kernel命名风格，纯C语言语法，模块和上下层充分解耦合。
2. 支持最大255个按键（理论无限个）。
3. 每个按键都可以设置是否启用，单独设置长按，双击的触发时间，电平是否反转。
4. 资源性能开销低，每个按键只占用28Byte RAM，使用状态机进行驱动。
5. 没有使用Delay和动态内存，整体基于时间戳判断，非常适合低性能的嵌入式平台。
6. 本模块可以用在STM32, 51, Arduino等所有嵌入式平台，满足多种项目的按键驱动需求。
7. 几乎不需要对模块代码进行任何修改即可使用。
8. 对RTOS应用具有良好的支持。

### 提示
1. 本模块建议放在人机交互层。
2. 本文件的返回值说明(int8_t): 返回0:正常,返回非0(-1等):错误。
3. 空闲时按键电平默认值反转(lv_reverse_t)说明: 如果发现按键按下就一直触发长按循环事件，初始化里反转这个位。
4. 转换条件(cdtn_t)说明:
5. 默认最大16个按键，可更改BTN_NUM宏进行修改。

### 使用方法
1. 创建一个回调函数用于给sGBD2读取按键电平 eg:
   ```
   bool get_lv(uint8_t btn_id){
       if(btn_id == 0){
           return !!HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0);
       }
       else if(btn_id == 1){
           return !!HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_1);
       }
       return false;
   }
   ```
2. 创建一个回调函数用于sGBD2触发事件时通知用户 eg:
   ```
   void trig(uint8_t btn_id,ev_flag_t btn_ev){...}
   ```
3. 创建一个初始化结构体，和HAL库类似: `btn_init_t btn_init;`
4. 对`btn_init`进行配置 eg:
   ```
   btn_init.en = 1;                // 使能此按键
   btn_init.lv_rev = lv_reverse;   // 空闲时的电平反转
   btn_init.dp_mode = dp_disable;  // 禁用双击,可提高连续单击速度
   btn_init.lp_loop_pridt = 500;   // 设置长按循环触发间隔每500ms触发一次
   btn_init.lp_trig_waitt = 2000;  // 设置长按触发时间2000ms
   btn_init.dp_prid_waitt = 200;   // 设置最大等待双击时间
   sGBD_SetAllBtnEnable(1);        // 设置所有按键使能(默认设置16个按键)
   sGBD_SetAllBtnMode(&btn_init);  // 装载btn_init的配置参数
   ```
5. 如果想要其中有几个按键模式不一样，可以参考HAL库初始化GPIO的操作:
   ```
   btn_init.dp_mode = dp_enable;   // 设置双击使能
   btn_init.lp_loop_pridt = 200;   // 长按循环触发时间为200ms一次
   sGBD_SetBtnMode(1,&btn_init);   // 装载参数
   ```
6. 使用`sGBD_Init(get_lv,trig,HAL_GetTick);`初始化sGBD2
7. 周期性调用处理函数:`sGBD_Handler();`

### 联系方式
- 知乎: https://www.zhihu.com/people/bad-boy17
- B站: https://space.bilibili.com/322702901
- GitHub: https://github.com/JackTang543
- QQ: 1583031618

By Sightseer. in home. 2024.01.31
