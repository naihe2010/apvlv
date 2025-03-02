function get_selection_offset(index) {
    let selection = window.getSelection();
    if (selection.rangeCount >= index) {
        let range = selection.getRangeAt(index);

        let start = document.createRange();
        start.setStart(document.documentElement, 0);
        start.setEnd(range.startContainer, range.startOffset);
        let startOffset = start.toString().length;

        let end = document.createRange();
        end.setStart(document.documentElement, 0);
        end.setEnd(range.endContainer, range.endOffset);
        let endOffset = end.toString().length;

        return [startOffset, endOffset];
    } else {
        return [null, null];
    }
}

function underline_by_offset(startOffset, endOffset) {
    // 获取文档的根元素
    const root = document.documentElement;
    let currentOffset = 0;
    let startNode = null;
    let endNode = null;
    let startNodeOffset = 0;
    let endNodeOffset = 0;

    // 递归遍历文档树，找到起始和结束节点
    function traverse(node) {
        if (node.nodeType === Node.TEXT_NODE) {
            const nodeLength = node.nodeValue.length;
            if (currentOffset <= startOffset && currentOffset + nodeLength > startOffset) {
                startNode = node;
                startNodeOffset = startOffset - currentOffset;
            }
            if (currentOffset <= endOffset && currentOffset + nodeLength > endOffset) {
                endNode = node;
                endNodeOffset = endOffset - currentOffset;
            }
            currentOffset += nodeLength;
        }
        for (let i = 0; i < node.childNodes.length; i++) {
            traverse(node.childNodes[i]);
        }
    }

    traverse(root);

    if (startNode && endNode) {
        if (startNode === endNode) {
            // 起始和结束节点相同
            const text = startNode.nodeValue;
            const beforeText = text.slice(0, startNodeOffset);
            const underlinedText = text.slice(startNodeOffset, endNodeOffset);
            const afterText = text.slice(endNodeOffset);

            const u = document.createElement('u');
            u.textContent = underlinedText;

            const parent = startNode.parentNode;
            const newBeforeText = document.createTextNode(beforeText);
            const newAfterText = document.createTextNode(afterText);

            parent.insertBefore(newBeforeText, startNode);
            parent.insertBefore(u, startNode);
            parent.insertBefore(newAfterText, startNode);
            parent.removeChild(startNode);
        } else {
            // 起始和结束节点不同
            // 处理起始节点
            const startText = startNode.nodeValue;
            const startBeforeText = startText.slice(0, startNodeOffset);
            const startUnderlinedText = startText.slice(startNodeOffset);

            const startU = document.createElement('u');
            startU.textContent = startUnderlinedText;

            const startParent = startNode.parentNode;
            const startNewBeforeText = document.createTextNode(startBeforeText);

            startParent.insertBefore(startNewBeforeText, startNode);
            startParent.insertBefore(startU, startNode);
            startParent.removeChild(startNode);

            // 处理中间节点
            let currentNode = startU.nextSibling;
            while (currentNode && currentNode!== endNode) {
                const nextNode = currentNode.nextSibling;
                const u = document.createElement('u');
                u.textContent = currentNode.nodeValue;
                currentNode.parentNode.replaceChild(u, currentNode);
                currentNode = nextNode;
            }

            // 处理结束节点
            const endText = endNode.nodeValue;
            const endUnderlinedText = endText.slice(0, endNodeOffset);
            const endAfterText = endText.slice(endNodeOffset);

            const endU = document.createElement('u');
            endU.textContent = endUnderlinedText;

            const endParent = endNode.parentNode;
            const endNewAfterText = document.createTextNode(endAfterText);

            endParent.insertBefore(endU, endNode);
            endParent.insertBefore(endNewAfterText, endNode);
            endParent.removeChild(endNode);
        }
    }
}