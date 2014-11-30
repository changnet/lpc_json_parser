/*
http://www.json.org
*/

#define JSON_BOOL    1
#define JSON_NULL    2
#define JSON_INT     3
#define JSON_FRAC    4
#define JSON_EXP     5
#define JSON_STRING  6

private string json_text = "";     //待解析的字符串
private string json_ch = "";       //当前解析字符
private int length = 0;            //json_text的长度
private int cur_pos = 0;           //当前解析位置
private mixed json_struct = 0;     //解析完成后的json数据结构，可能为array或mapping

private mixed *parse_array();
private mapping parse_object();

/*
读取json_txt中的某个字符,越界返回空字串
使用子字符串运算 (substring operation)　 从一个字符串变量中, 取出字符串的一部份 (substring)
*/

private string json_ch_pos( int pos )
{
    return json_text[pos..pos];
}

private void init_parse( string _json )
{
     json_text = _json;
    length = strlen( json_text );
    if ( 0 == length )
    {
        json_ch = "";
        return;
    }
    
    json_text = _json;
    

    cur_pos = 0;    //复位
    json_ch = json_text[0..0];
}

private int get_value_type( string value )
{
    if ( "true" == value || "false" == value )
        return JSON_BOOL;
    
    if ( "null" == value )
        return JSON_NULL;
    
    if ( regexp(value,"^[+-]?[0-9]*$") )
        return JSON_INT;

    if ( regexp(value,"^[+-]?[0-9]+[\.][0-9]+$") )    //fraction
        return JSON_FRAC;
    
    //正则表达式中，小括号中的连续字符作为可选
    if ( regexp(value,"^[+-]?[0-9]+([\.][0-9]+)?[eE]+[+-]?[0-9]+$") )   //exponent
        return JSON_EXP;
    
    return JSON_STRING;  //严格按json的话，字符串需要""

    //0.345E+2表示0.345×10的2次方，是科学计数法。中间的表示10的正次幂，-表示负此幂，就是34.5，-34.4E-3是-0.0344
    //123E-1是12.3，12.E2是1200，1.E-3是0.001

}

/* 将指数转换为float */
private float parse_exponent( string value )
{
    string ch = "";
    string frac_num = "";
    string pow_num = "";
    int length = strlen(value);
    for ( int i = 0;i < length;i ++ )  //LPC没有正则表达式可以取子字符串?
    {
        ch = value[i..i];
        if ( "e" == ch || "E" == ch )
        {
            frac_num = value[0..i];
            pow_num  = value[1+i..length-1];  //越界将取到空串
            
            break;
        }
    }
    
    if ( "" == frac_num || "" == pow_num )  //not a exponent string
        return 0.00;
    
    return pow( to_float(frac_num),to_float(pow_num) );
}

/* 解析json中的\u特殊字符 */
private string parse_hex_string( string value )
{
    int ch = 0;
    int ch_value = 0;
    int length = strlen( value );  //should be 4
    for ( int i = 0;i < length;i ++ )  //lpc中没有直接转换的方式，需要自己转换
    {
        return value;
    }
}

/* 解析字符串，处理转义字符 */
private string parse_string( string value )
{
    int backslash = 0;
    string ch = "";
    string str = "";
    int length = strlen(value);

    if ( length < 2 || "\"" != value[0..0] || "\"" != value[length-1..length-1] )  //not a valid json string,abort parse
        return value;
    
    for ( int i = 1;i < length-1;i ++ )    //skip first " and last "
    {
        ch = value[i..i];
        if (backslash)  //与转义搭配的字符
        {
            switch(ch)  
            {  
            case "\"":  
                ch = "\"";  
                break;  
            case "\"":  
                ch = "\"";  
                break;  
            case "\\":  
                ch = "\\";  
                break;  
            case "/":  
                ch = "/";  
                break;  
            case "b":  
                ch = "\b";  
                break;  
            case "f":  
                ch = "\f";  
                break;  
            case "n":  
                ch = "\n";  
                break;  
            case "r":  
                ch = "\r";  
                break;  
            case "t":  
                ch = "\t";  
                break;  
            case "u":  
                ch = parse_hex_string(value[1+i..i+4]);
                break;
            }
            
           backslash = 0;  //转义处理完成 
        }
        else if ( "\\" == ch )  //遇到转义
        {
            backslash = 1;
            continue;
        }
        
        str += ch;
    }
    
    return str;
}

/* 指向下一个字符 */
private void json_ch_next()
{
    cur_pos++;
    json_ch = json_text[cur_pos..cur_pos];
}

/* 将一个字符串转换为具体类型 */
private mixed parse_type( string value )
{
    switch ( get_value_type( value ) )  /* 允许key为多种数据类型 */
    {
    case JSON_BOOL   :
        if ( "true" == value )
            return 1;
        return 0;
        break;
    case JSON_NULL   : return 0;break;
    case JSON_INT    : return to_int(value);break;
    case JSON_FRAC   : return to_float(value);break;
    case JSON_EXP    : return parse_exponent(value);break;
    case JSON_STRING : return parse_string(value);break;
    }

    return 0;  
}

/* 跳过空格、tab键 */
private void skip_blank()
{
    do
    {
        if ( " " == json_ch || "\t" == json_ch || "\n" == json_ch )
        {
            json_ch_next();
            continue;
        }
        
        return;
        
    }while ( "" != json_ch );
}

/* 读取key字符,如果是字符串，则包含"" */
private string read_key_string()
{
    string key = "";
        
    while (1)
    {
        if ( "" != json_ch && ":" != json_ch )
        {
            key += json_ch;
            json_ch_next();
        }
        else
            break;
    }
    
    return key;
}

/* 读取以"开始、结束的字符串 */
private string read_string()
{
    string value = "";
    
    json_ch_next();  //read first "
        
    while (1)
    {
        if ( "\"" == json_ch  )  //字符结束
            break;
        
        value += json_ch;
        json_ch_next();
    }
    
    json_ch_next();  //read last "
    
    return value;
}

/* 读取value字符串，但不包括value为array或object的情况 */
private string read_value_string()
{
    string value = "";
        
    if ( "\"" == json_ch )    /* 字符串类型特别处理，它可包括空格、tab键...扰乱其他类型的读取 */
        return read_string();
        
    while (1)
    { 
        /* 字符越界，array元素结束，array最后一个元素结束，object结束，空格、换行符 */
        if ( "" == json_ch || "," == json_ch || "]" == json_ch || "}" == json_ch )
            break;
        else if ( " " == json_ch || "\n" == json_ch )/* 由于格式化原因，部分value结束后可能用空格、换行符结束 */
        {
            break;
        }
        
        value += json_ch;
        json_ch_next();
    }
    
    return value;
}

/* 解析对象结构中的key,这里 */
private mixed parse_key()
{
    string key = read_key_string();

    return parse_type( key );    
}

/* 解析array或object的值 */
private mixed parse_value()
{
    if ( "[" == json_ch )  //value是一个数组
        return parse_array();
    else if ( "{" == json_ch )    //value是一个object
        return parse_object();
    else
    {
        string value = read_value_string();  //true false null number
        return parse_type( value );
    }
}

/* 解析数组结构 */
private mixed *parse_array()
{
    mixed *_struct = ({});
    
    json_ch_next();  //read [
    
    while ( "]" != json_ch )
    {
        skip_blank();
        
        _struct += ({ parse_value() });
        
        skip_blank();
        
        if ( "," == json_ch ) //还有下一个元素，跳过,继续读取
        {
            json_ch_next();
        }  
    }
    
    json_ch_next();  //read ]
        
    return _struct;
}

/* 解析对象结构 key-value */
private mapping parse_object()
{
    mixed key = "";
    mixed value = 0;
    
    mapping _struct = ([]);
    
    json_ch_next();  //read {
    
    while ( "}" != json_ch )
    {
        skip_blank();
        
        key = parse_key();
        
        skip_blank();

        json_ch_next();  //read : after key
        
        skip_blank();
            
        value = parse_value();
        
        skip_blank();
        
        _struct[key] = value;
        
        if ( "," == json_ch )    //read , after value
            json_ch_next();
    }
    
    json_ch_next();  //read }
    
    return _struct;
}

/* return array or mapping,error return 0 */
mixed parse(string _json)
{
    init_parse( _json );

    skip_blank();

    if ( "[" == json_ch ) //array struct
    {
        return json_struct = parse_array();
    }
    else if ( "{" == json_ch )   //object struct
    {
        return json_struct = parse_object();
    }
    else
        return 0;      //not a valid json string
}
