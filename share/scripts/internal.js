// internal.js
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

    const beforeText = textContent.slice(0, startOffset);
    const underlinedText = textContent.slice(startOffset, validEndOffset);
    const afterText = textContent.slice(validEndOffset);

    const underlineElement = document.createElement('u');
    underlineElement.textContent = underlinedText;

    const fragment = document.createDocumentFragment();
    if (beforeText) fragment.appendChild(document.createTextNode(beforeText));
    fragment.appendChild(underlineElement);
    if (afterText) fragment.appendChild(document.createTextNode(afterText));

    parent.replaceChild(fragment, textNode);
}

function traverseTextNodes(root, callback) {
    const walker = document.createTreeWalker(root, NodeFilter.SHOW_TEXT, null,);

    let node;
    while ((node = walker.nextNode())) {
        if (callback(node) === false) break;
    }
}

function underlineByOffset(startOffset, endOffset) {
    if (startOffset >= endOffset || startOffset < 0) {
        throw new Error('Invalid offset range');
    }

    let currentOffset = 0;
    const nodesInfo = {
        start: {node: null, offset: 0}, end: {node: null, offset: 0}, between: []
    };

    traverseTextNodes(document.documentElement, (textNode) => {
        const nodeLength = textNode.nodeValue.length;
        const nodeEnd = currentOffset + nodeLength;

        if (!nodesInfo.start.node && currentOffset <= startOffset && nodeEnd > startOffset) {
            nodesInfo.start.node = textNode;
            nodesInfo.start.offset = startOffset - currentOffset;
        }

        if (!nodesInfo.end.node && currentOffset <= endOffset && nodeEnd > endOffset) {
            nodesInfo.end.node = textNode;
            nodesInfo.end.offset = endOffset - currentOffset;
            return false;
        }

        if (nodesInfo.start.node && !nodesInfo.end.node && textNode !== nodesInfo.start.node) {
            nodesInfo.between.push(textNode);
        }

        currentOffset = nodeEnd;
        return true;
    });

    if (nodesInfo.start.node && nodesInfo.end.node) {
        underlineTextNode(nodesInfo.start.node, nodesInfo.start.offset, nodesInfo.start.node === nodesInfo.end.node ? nodesInfo.end.offset : -1);

        nodesInfo.between.forEach(node => {
            underlineTextNode(node, 0);
        });

        if (nodesInfo.start.node !== nodesInfo.end.node) {
            underlineTextNode(nodesInfo.end.node, 0, nodesInfo.end.offset);
        }
    }
}

function scrollByTimes(times, h, v) {
    window.scrollBy(times * h, times * v);
}

function scrollToAnchor(anchor) {
    if (typeof anchor === 'string' && anchor.startsWith('#')) {
        const element = document.getElementById(anchor.substr(1));
        if (element) {
            element.scrollIntoView();
        }
    }
}

function scrollToPosition(xrate, yrate) {
    const x = window.screenX * xrate;
    const y = (document.body.offsetHeight - window.innerHeight) * yrate;
    window.scroll(x, y);
}

function dispatchKeydownEvent(keyCode) {
    const event = new KeyboardEvent('keydown', {
        keyCode: keyCode, bubbles: true, cancelable: true
    });
    document.dispatchEvent(event);
}