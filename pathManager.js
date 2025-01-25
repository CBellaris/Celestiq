import path from 'path';
import { fileURLToPath } from 'url';

// 获取 main.js 所在路径
const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// 导出相对于 main.js 的根路径
export const appRoot = __dirname;

// 定义便捷函数
export const resolveAppPath = (...relativePath) => path.resolve(appRoot, ...relativePath);
