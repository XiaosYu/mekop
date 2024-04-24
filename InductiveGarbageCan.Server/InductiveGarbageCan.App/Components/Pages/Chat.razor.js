﻿import EventHandler from "../../_content/BootstrapBlazor/modules/event-handler.js?v=$version"

export function init(id) {
    const el = document.getElementById(id)

    EventHandler.on(el, 'keyup', e => {
        if (e.key === 'Enter' && !e.shiftKey && el.value.trim('\n').length > 0) {
            el.blur()
            document.querySelector('.btn-send').click()
        }
    })
}



export function scroll(id) {
    const body = document.querySelector('.chat-body')
    if (body.offsetHeight < body.scrollHeight) {
        body.scrollTop = body.scrollHeight
    }

    const el = document.getElementById(id)
    if (el) {
        //el.focus()
    }
}



export function dispose(id) {
    const el = document.getElementById(id)
    EventHandler.off(el, 'onkeyup')
}
