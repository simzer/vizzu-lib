import Render from './render.js'
import Events from './events.js'
import Data from './data.js'
import { Animation, AnimControl } from './animcontrol.js'
import Tooltip from './tooltip.js'
import Presets from './presets.js'
import VizzuModule from './cvizzu.js'
import { getCSSCustomPropsForElement, propsToObject } from './utils.js'
import ObjectRegistry from './objregistry.js'

let vizzuOptions = null

class Snapshot {}

class CChart {}

class CCanvas {}

export default class Vizzu {
  static get presets() {
    return new Presets()
  }

  static options(options) {
    vizzuOptions = options
  }

  constructor(container, initState) {
    this._container = container

    if (!(this._container instanceof HTMLElement)) {
      this._container = document.getElementById(container)
    }

    if (!this._container) {
      throw new Error(`Cannot find container ${this._container} to render Vizzu!`)
    }

    this._propPrefix = 'vizzu'
    this._started = false

    this._resolveAnimate = null
    this.initializing = new Promise((resolve) => {
      this._resolveAnimate = resolve
    })
    this.anim = this.initializing

    const moduleOptions = {}

    if (vizzuOptions?.wasmUrl) {
      moduleOptions.locateFile = function (path) {
        if (path.endsWith('.wasm')) {
          return vizzuOptions.wasmUrl
        }
        return path
      }
    }

    // load module
    VizzuModule(moduleOptions).then((module) => {
      if (this._resolveAnimate) {
        this._resolveAnimate(this._init(module))
      }
    })

    if (initState) {
      this.initializing = this.animate(initState, 0)
    }
  }

  _callOnChart(f) {
    return this._call(f).bind(this, this._cChart.id)
  }

  _call(f) {
    return (...params) => {
      try {
        return f(...params)
      } catch (e) {
        if (Number.isInteger(e)) {
          const address = parseInt(e, 10)
          const type = new this.module.ExceptionInfo(address).get_type()
          const cMessage = this.module._vizzu_errorMessage(address, type)
          const message = this.module.UTF8ToString(cMessage)
          throw new Error('error: ' + message)
        } else {
          throw e
        }
      }
    }
  }

  _iterateObject(obj, paramHandler, path = '') {
    if (obj) {
      Object.keys(obj).forEach((key) => {
        const newPath = path + (path.length === 0 ? '' : '.') + key
        if (obj[key] !== null && typeof obj[key] === 'object') {
          this._iterateObject(obj[key], paramHandler, newPath)
        } else {
          this._setValue(newPath, obj[key], paramHandler)
        }
      })
    }
  }

  /* Note: If the value string containing a JSON, it will be parsed. */
  _setNestedProp(obj, path, value) {
    const propList = path.split('.')
    propList.forEach((prop, i) => {
      if (i < propList.length - 1) {
        obj[prop] = obj[prop] || (typeof propList[i + 1] === 'number' ? [] : {})
        obj = obj[prop]
      } else {
        obj[prop] = value.startsWith('[') || value.startsWith('{') ? JSON.parse(value) : value
      }
    })
  }

  _setValue(path, value, setter) {
    if (path !== '' + path) {
      throw new Error('first parameter should be string')
    }

    const cpath = this._toCString(path)
    const cvalue = this._toCString(String(value).toString())

    try {
      setter(cpath, cvalue)
    } finally {
      this.module._free(cvalue)
      this.module._free(cpath)
    }
  }

  _setStyle(style) {
    this._iterateObject(style, (path, value) => {
      this._callOnChart(this.module._style_setValue)(path, value)
    })
  }

  _cloneObject(lister, getter, ...args) {
    const clistStr = this._call(lister)()
    const listStr = this._fromCString(clistStr)
    const list = JSON.parse(listStr)
    const res = {}
    for (const path of list) {
      const cpath = this._toCString(path)
      let cvalue
      try {
        cvalue = this._callOnChart(getter)(cpath, ...args)
        const value = this._fromCString(cvalue)
        this._setNestedProp(res, path, value)
      } finally {
        this.module._free(cpath)
      }
    }
    Object.freeze(res)
    return res
  }

  _recursiveCopy(obj) {
    if (obj === null || typeof obj !== 'object') {
      return obj
    }

    if (obj instanceof Function) {
      return obj
    }

    if (obj instanceof Array) {
      const copyArray = []
      obj.map((arrayElement) => copyArray.push(arrayElement))
      return copyArray
    }

    const copyObj = {}
    for (const key in obj) {
      if (key in obj) {
        copyObj[key] = this._recursiveCopy(obj[key])
      }
    }
    return copyObj
  }

  get config() {
    this._validateModule()
    return this._cloneObject(this.module._chart_getList, this.module._chart_getValue)
  }

  get style() {
    this._validateModule()
    return this._cloneObject(this.module._style_getList, this.module._style_getValue, false)
  }

  getComputedStyle() {
    this._validateModule()
    return this._cloneObject(this.module._style_getList, this.module._style_getValue, true)
  }

  get data() {
    this._validateModule()
    const cInfo = this._callOnChart(this.module._data_metaInfo)()
    const info = this._fromCString(cInfo)
    return { series: JSON.parse(info) }
  }

  _setConfig(config) {
    if (config !== null && typeof config === 'object') {
      Object.keys(config).forEach((key) => {
        if (this._channelNames.includes(key)) {
          config.channels = config.channels || {}
          config.channels[key] = config[key]
          delete config[key]
        }
      })
    }

    if (config?.channels) {
      const channels = config.channels
      Object.keys(channels).forEach((ch) => {
        if (typeof channels[ch] === 'string') {
          channels[ch] = [channels[ch]]
        }

        if (channels[ch] === null || Array.isArray(channels[ch])) {
          channels[ch] = { set: channels[ch] }
        }

        if (typeof channels[ch].attach === 'string') {
          channels[ch].attach = [channels[ch].attach]
        }

        if (typeof channels[ch].detach === 'string') {
          channels[ch].detach = [channels[ch].detach]
        }

        if (typeof channels[ch].set === 'string') {
          channels[ch].set = [channels[ch].set]
        }

        if (Array.isArray(channels[ch].set) && channels[ch].set.length === 0) {
          channels[ch].set = null
        }
      })
    }

    this._iterateObject(config, (path, value) => {
      this._callOnChart(this.module._chart_setValue)(path, value)
    })
  }

  on(eventName, handler) {
    this._validateModule()
    this.events.add(eventName, handler)
  }

  off(eventName, handler) {
    this._validateModule()
    this.events.remove(eventName, handler)
  }

  store() {
    this._validateModule()
    return this._objectRegistry.get(this._callOnChart(this.module._chart_store), Snapshot)
  }

  feature(name, enabled) {
    this._validateModule()
    if (name === 'tooltip') {
      this._tooltip.enable(enabled)
    } else if (name === 'logging') {
      this._call(this.module._vizzu_setLogging)(enabled)
    } else if (name === 'rendering') {
      this.render.enabled = enabled
    }
  }

  _validateModule() {
    if (!this.module) {
      throw new Error('Vizzu not initialized. Use `initializing` promise.')
    }
  }

  animate(...args) {
    const copiedArgs = this._recursiveCopy(args)
    let activate
    const activated = new Promise((resolve, reject) => {
      activate = resolve
    })
    this.anim = this.anim.then(() => this._animate(copiedArgs, activate))
    this.anim.activated = activated
    return this.anim
  }

  _animate(args, activate) {
    const anim = new Promise((resolve, reject) => {
      const callbackPtr = this.module.addFunction((ok) => {
        if (ok) {
          resolve(this)
        } else {
          // eslint-disable-next-line prefer-promise-reject-errors
          reject('animation canceled')
          this.anim = Promise.resolve(this)
        }
        this.module.removeFunction(callbackPtr)
      }, 'vi')
      this._processAnimParams(...args)
      this._callOnChart(this.module._chart_animate)(callbackPtr)
    }, this)
    activate(new AnimControl(this))
    return anim
  }

  _processAnimParams(animTarget, animOptions) {
    if (animTarget instanceof Animation) {
      this._callOnChart(this.module._chart_anim_restore)(animTarget.id)
    } else {
      const anims = []

      if (Array.isArray(animTarget)) {
        for (const target of animTarget)
          if (target.target !== undefined)
            anims.push({ target: target.target, options: target.options })
          else anims.push({ target, options: undefined })
      } else {
        anims.push({ target: animTarget, options: animOptions })
      }

      for (const anim of anims) this._setKeyframe(anim.target, anim.options)
    }
    this._setAnimation(animOptions)
  }

  _setKeyframe(animTarget, animOptions) {
    if (animTarget) {
      if (animTarget instanceof Snapshot) {
        this._callOnChart(this.module._chart_restore)(animTarget.id)
      } else {
        let obj = Object.assign({}, animTarget)

        if (!obj.data && obj.style === undefined && !obj.config) {
          obj = { config: obj }
        }

        this._data.set(obj.data)

        // setting style, including CSS properties
        if (obj.style === null) {
          obj.style = { '': null }
        }
        const style = JSON.parse(JSON.stringify(obj.style || {}))
        const props = getCSSCustomPropsForElement(this._container, this._propPrefix)
        this._setStyle(propsToObject(props, style, this._propPrefix))
        this._setConfig(Object.assign({}, obj.config))
      }
    }

    this._setAnimation(animOptions)

    this._callOnChart(this.module._chart_setKeyframe)()
  }

  _setAnimation(animOptions) {
    if (typeof animOptions !== 'undefined') {
      if (animOptions === null) {
        animOptions = { duration: 0 }
      } else if (typeof animOptions === 'string' || typeof animOptions === 'number') {
        animOptions = { duration: animOptions }
      }

      if (typeof animOptions === 'object') {
        animOptions = Object.assign({}, animOptions)
        this._iterateObject(animOptions, (path, value) => {
          this._callOnChart(this.module._anim_setValue)(path, value)
        })
      } else {
        throw new Error('invalid animation option')
      }
    }
  }

  // keeping this one for backward compatibility
  get animation() {
    return new AnimControl(this)
  }

  version() {
    this._validateModule()
    return this.module.UTF8ToString(this.module._vizzu_version())
  }

  getCanvasElement() {
    return this.canvas
  }

  forceUpdate() {
    this._validateModule()
    this.render.updateFrame(true)
  }

  _start() {
    if (!this._started) {
      this.render.updateFrame()

      this._updateInterval = setInterval(() => {
        this.render.updateFrame()
      }, 25)

      this._started = true
    }
  }

  _toCString(str) {
    const len = str.length * 4 + 1
    const buffer = this.module._malloc(len)
    this.module.stringToUTF8(str, buffer, len)
    return buffer
  }

  _fromCString(str) {
    return this.module.UTF8ToString(str)
  }

  _init(module) {
    this.module = module
    this.module.callback = this._call(this.module._callback)

    this.canvas = this._createCanvas()

    this.render = new Render()
    this._data = new Data(this)
    this.events = new Events(this)
    this.module.events = this.events
    this._tooltip = new Tooltip(this)
    this._objectRegistry = new ObjectRegistry(this._call(this.module._object_free))
    this._cChart = this._objectRegistry.get(this._call(this.module._vizzu_createChart), CChart)

    const ccanvas = this._objectRegistry.get(this._call(this.module._vizzu_createCanvas), CCanvas)
    this.render.init(ccanvas, this._callOnChart(this.module._vizzu_update), this.canvas, false)
    this.module.renders = this.module.renders || {}
    this.module.renders[ccanvas.id] = this.render

    this._call(this.module._vizzu_setLogging)(false)
    this._channelNames = Object.keys(this.config.channels)
    this._setupDOMEventHandlers(this.canvas)

    this._start()

    return this
  }

  _createCanvas() {
    let canvas = null
    const placeholder = this._container

    if (placeholder instanceof HTMLCanvasElement) {
      canvas = placeholder
    } else {
      canvas = document.createElement('CANVAS')
      canvas.style.width = '100%'
      canvas.style.height = '100%'
      placeholder.appendChild(canvas)
    }

    if (!(canvas instanceof HTMLCanvasElement)) {
      throw new Error('Error initializing <canvas> for Vizzu!')
    }

    return canvas
  }

  _setupDOMEventHandlers(canvas) {
    this._resizeObserver = new ResizeObserver(() => {
      this.render.updateFrame(true)
    })

    this._resizeObserver.observe(canvas)

    this._pointermoveHandler = (evt) => {
      const pos = this.render.clientToRenderCoor({ x: evt.clientX, y: evt.clientY })
      this._callOnChart(this.module._vizzu_pointerMove)(
        this.render.ccanvas.id,
        evt.pointerId,
        pos.x,
        pos.y
      )
    }

    this._pointerupHandler = (evt) => {
      const pos = this.render.clientToRenderCoor({ x: evt.clientX, y: evt.clientY })
      this._callOnChart(this.module._vizzu_pointerUp)(
        this.render.ccanvas.id,
        evt.pointerId,
        pos.x,
        pos.y
      )
    }

    this._pointerdownHandler = (evt) => {
      const pos = this.render.clientToRenderCoor({ x: evt.clientX, y: evt.clientY })
      this._callOnChart(this.module._vizzu_pointerDown)(
        this.render.ccanvas.id,
        evt.pointerId,
        pos.x,
        pos.y
      )
    }

    this._pointerleaveHandler = (evt) => {
      this._callOnChart(this.module._vizzu_pointerLeave)(this.render.ccanvas.id, evt.pointerId)
    }

    this._wheelHandler = (evt) => {
      this._callOnChart(this.module._vizzu_wheel)(this.render.ccanvas.id, evt.deltaY)
    }

    this._keydownHandler = (evt) => {
      let key = evt.keyCode <= 255 ? evt.keyCode : 0
      const keys = [33, 34, 36, 35, 37, 39, 38, 40, 27, 9, 13, 46]
      for (let i = 0; i < keys.length; i++) {
        if (evt.key === keys[i]) {
          key = 256 + i
        }
      }
      if (key !== 0) {
        this._callOnChart(this.module._vizzu_keyPress)(
          this.render.ccanvas.id,
          key,
          evt.ctrlKey,
          evt.altKey,
          evt.shiftKey
        )
      }
    }

    canvas.addEventListener('pointermove', this._pointermoveHandler)
    canvas.addEventListener('pointerup', this._pointerupHandler)
    canvas.addEventListener('pointerdown', this._pointerdownHandler)
    canvas.addEventListener('pointerleave', this._pointerleaveHandler)
    canvas.addEventListener('wheel', this._wheelHandler)
    document.addEventListener('keydown', this._keydownHandler)
  }

  detach() {
    this?._resizeObserver.disconnect()
    if (this._updateInterval) clearInterval(this._updateInterval)
    if (this._pointermoveHandler)
      this?.canvas.removeEventListener('pointermove', this._pointermoveHandler)
    if (this._pointerupHandler)
      this?.canvas.removeEventListener('pointerup', this._pointerupHandler)
    if (this._pointerdownHandler)
      this?.canvas.removeEventListener('pointerdown', this._pointerdownHandler)
    if (this._pointerleaveHandler)
      this?.canvas.removeEventListener('pointerleave', this._pointerleaveHandler)
    if (this._wheelHandler) this?.canvas.removeEventListener('wheel', this._wheelHandler)
    if (this._keydownHandler) document.removeEventListener('keydown', this._keydownHandler)
    if (this._container && this._container !== this.canvas) this._container.removeChild(this.canvas)
  }

  getConverter(target, from, to) {
    this._validateModule()
    if (target === 'plot-area') {
      if (from === 'relative' || to === 'canvas') return this._toCanvasCoords.bind(this)
      if (from === 'canvas' || to === 'relative') return this._toRelCoords.bind(this)
    }
    return null
  }

  _toCanvasCoords(point) {
    const ptr = this._callOnChart(this.module._chart_relToCanvasCoords)(point.x, point.y)
    return {
      x: this.module.getValue(ptr, 'double'),
      y: this.module.getValue(ptr + 8, 'double')
    }
  }

  _toRelCoords(point) {
    const ptr = this._callOnChart(this.module._chart_canvasToRelCoords)(point.x, point.y)
    return {
      x: this.module.getValue(ptr, 'double'),
      y: this.module.getValue(ptr + 8, 'double')
    }
  }
}
