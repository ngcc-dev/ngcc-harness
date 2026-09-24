import argparse
import logging
import os
from datetime import datetime

def setup_logging(log_file):
# 确保日志目录存在
    log_dir = os.path.dirname(log_file)
    if log_dir and not os.path.exists(log_dir):
        os.makedirs(log_dir)

    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s - %(levelname)s - %(message)s',
        handlers=[
            logging.FileHandler(log_file, encoding='utf-8'),
            logging.StreamHandler()
        ]
    )
    return logging.getLogger(__name__)

def normalize_data(input_data, divisor):
    """
    执行归一化计算：输入数据 / 参数
    支持列表、元组或单个数值
    """
    if divisor == 0:
        raise ValueError("除数 (divisor) 不能为 0")
    
    if isinstance(input_data, (list, tuple)):
        return [x / divisor for x in input_data]
    else:
        return input_data / divisor

def parse_input_data(input_str):
    """
    解析输入数据字符串
    支持格式：
    1. 逗号分隔的数字: "1,2,3,4"
    2. 单个数字: "10"
    """
    try:
        if ',' in input_str:
            return [float(x.strip()) for x in input_str.split(',')]
        else:
            return float(input_str)
    except ValueError:
        raise ValueError("输入数据格式错误，请输入数字或逗号分隔的数字列表")

def main():
    # 设置命令行参数解析
    parser = argparse.ArgumentParser(description='数据归一化计算脚本 (输入数据 / 参数)')
    parser.add_argument('--input', '-i', type=str, required=True, 
                        help='输入数据 (单个数字或逗号分隔的列表，例如: "1,2,3" 或 "10")')
    parser.add_argument('--divisor', '-d', type=float, required=True, 
                        help='归一化除数参数')
    parser.add_argument('--output-log', '-o', type=str, default='normalization_log.txt', 
                        help='输出日志文件路径 (默认: normalization_log.txt)')
    
    args = parser.parse_args()

    # 初始化日志
    logger = setup_logging(args.output_log)
    logger.info("="*50)
    logger.info("开始归一化计算任务")
    logger.info(f"原始输入数据: {args.input}")
    logger.info(f"归一化除数: {args.divisor}")

    try:
        # 解析输入数据
        data = parse_input_data(args.input)
        logger.info(f"解析后的数据: {data}")

        # 执行归一化
        result = normalize_data(data, args.divisor)
        
        # 记录结果
        logger.info(f"归一化计算完成")
        logger.info(f"归一化结果: {result}")
        
        # 计算统计信息 (如果是列表)
        if isinstance(result, list):
            logger.info(f"结果数量: {len(result)}")
            logger.info(f"结果最小值: {min(result)}")
            logger.info(f"结果最大值: {max(result)}")
            logger.info(f"结果平均值: {sum(result)/len(result)}")

        logger.info("任务成功结束")
        print(f"\n归一化结果: {result}")
        print(f"详细日志已保存至: {os.path.abspath(args.output_log)}")

    except Exception as e:
        logger.error(f"任务执行失败: {str(e)}", exc_info=True)
        print(f"错误: {str(e)}")
        return 1
    
    return 0

if __name__ == "__main__":
    exit(main())
