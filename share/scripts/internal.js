// internal.js
/**
 * 获取指定索引选区对应的全局文本偏移量
 * @param {number} index - 选区索引
 * @returns {[number|null, number|null]} 起始和结束偏移量
 */
function getSelectionOffset(index) {
    const selection = window.getSelection();

    if (!selection.rangeCount || index >= selection.rangeCount) {
        return [null, null];
    }

    const createOffsetRange = (container, offset) => {
        const range = document.createRange();
        range.setStart(document.documentElement, 0);
        range.setEnd(container, offset);
        return range.toString().length;
    };

    try {
        const range = selection.getRangeAt(index);
        return [createOffsetRange(range.startContainer, range.startOffset), createOffsetRange(range.endContainer, range.endOffset)];
    } catch (error) {
        console.error('Error accessing selection range:', error);
        return [null, null];
    }
}

/**
 * 在指定文本节点创建下划线
 * @param {Node} textNode - 文本节点
 * @param {number} startOffset - 起始偏移量
 * @param {number} [endOffset=-1] - 结束偏移量
 */
function underlineTextNode(textNode, startOffset, endOffset = -1) {
    if (!(textNode instanceof Text)) {
        throw new Error('Invalid text node provided');
    }

    const textContent = textNode.nodeValue;
    const validEndOffset = endOffset === -1 ? textContent.length : endOffset;

    if (startOffset < 0 || validEndOffset > textContent.length || startOffset > validEndOffset) {
        throw new Error('Invalid offset values');
    }

    const parent = textNode.parentNode;
    if (!parent) {
        throw new Error('Text node has no parent element');
    }

    // 分割原始文本节点
    const beforeText = textContent.slice(0, startOffset);
    const underlinedText = textContent.slice(startOffset, validEndOffset);
    const afterText = textContent.slice(validEndOffset);

    // 创建新节点
    const underlineElement = document.createElement('u');
    underlineElement.textContent = underlinedText;

    // 批量DOM操作
    const fragment = document.createDocumentFragment();
    if (beforeText) fragment.appendChild(document.createTextNode(beforeText));
    fragment.appendChild(underlineElement);
    if (afterText) fragment.appendChild(document.createTextNode(afterText));

    parent.replaceChild(fragment, textNode);
}

/**
 * 递归遍历文本节点
 * @param {Node} root - 根节点
 * @param {Function} callback - 回调函数
 */
function traverseTextNodes(root, callback) {
    const walker = document.createTreeWalker(root, NodeFilter.SHOW_TEXT, null,);

    let node;
    while ((node = walker.nextNode())) {
        if (callback(node) === false)
            break;
    }
}

/**
 * 根据全局偏移量添加下划线
 * @param {number} startOffset - 全局起始偏移量
 * @param {number} endOffset - 全局结束偏移量
 */
function underlineByOffset(startOffset, endOffset) {
    if (startOffset >= endOffset || startOffset < 0) {
        throw new Error('Invalid offset range');
    }

    let currentOffset = 0;
    const nodesInfo = {
        start: {node: null, offset: 0}, end: {node: null, offset: 0}, between: []
    };

    // 单次遍历收集所有必要信息
    traverseTextNodes(document.documentElement, (textNode) => {
        const nodeLength = textNode.nodeValue.length;
        const nodeEnd = currentOffset + nodeLength;

        // 检测起始节点
        if (!nodesInfo.start.node && currentOffset <= startOffset && nodeEnd > startOffset) {
            nodesInfo.start.node = textNode;
            nodesInfo.start.offset = startOffset - currentOffset;
        }

        // 检测结束节点
        if (!nodesInfo.end.node && currentOffset <= endOffset && nodeEnd > endOffset) {
            nodesInfo.end.node = textNode;
            nodesInfo.end.offset = endOffset - currentOffset;
            return false;
        }

        // 收集中间节点
        if (nodesInfo.start.node && !nodesInfo.end.node && textNode !== nodesInfo.start.node) {
            nodesInfo.between.push(textNode);
        }

        currentOffset = nodeEnd;
        return true;
    });

    // 处理下划线逻辑
    if (nodesInfo.start.node && nodesInfo.end.node) {
        // 处理起始节点
        underlineTextNode(nodesInfo.start.node, nodesInfo.start.offset, nodesInfo.start.node === nodesInfo.end.node ? nodesInfo.end.offset : -1);

        // 处理中间完整节点
        nodesInfo.between.forEach(node => {
            underlineTextNode(node, 0);
        });

        // 处理结束节点（当与起始节点不同时）
        if (nodesInfo.start.node !== nodesInfo.end.node) {
            underlineTextNode(nodesInfo.end.node, 0, nodesInfo.end.offset);
        }
    }
}