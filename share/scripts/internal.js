// 115452,115532
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

function underline_node_offset(container, startOffset, endOffset = -1) {
    console.log("container: " + container + " start: " + startOffset + " end: " + endOffset);
    const text = container.nodeValue;

    if (endOffset === -1) {
        endOffset = text.length;
    }

    const beforeText = text.slice(0, startOffset);
    const underlinedText = text.slice(startOffset, endOffset);
    const afterText = text.slice(endOffset);

    const u = document.createElement('u');
    u.textContent = underlinedText;

    const parent = container.parentNode;

    if (beforeText.length > 0) {
        const beforeTextNode = document.createTextNode(beforeText);
        parent.insertBefore(beforeTextNode, container);
    }

    parent.insertBefore(u, container);

    container.nodeValue = afterText;
}

function traverse(node, type, func) {
    if (node.nodeType === type) {
        func(node);
    }

    for (let i = 0; i < node.childNodes.length; i++) {
        traverse(node.childNodes[i], type, func);
    }
}

function underline_by_offset(startOffset, endOffset) {
    const root = document.documentElement;
    let currentOffset = 0;
    let startNode = null;
    let endNode = null;
    let startNodeOffset = 0;
    let endNodeOffset = 0;

    // 递归遍历文档树，找到起始和结束节点
    traverse(root, Node.TEXT_NODE, (node) => {
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
    });

    if (startNode && endNode) {
        if (startNode === endNode) {
            underline_node_offset(startNode, startNodeOffset, endNodeOffset);
            return;
        }

        let inUnderline = false;
        let nodes = [];
        traverse(root, Node.TEXT_NODE, (node) => {
            if (node === startNode) {
                inUnderline = true;
            } else if (node === endNode) {
                inUnderline = false;
            } else if (inUnderline) {
                nodes[nodes.length] = node;
            }
        });

        underline_node_offset(startNode, startNodeOffset);
        for (let node of nodes) {
            underline_node_offset(node, 0);
        }
        underline_node_offset(endNode, 0, endNodeOffset);
    }
}