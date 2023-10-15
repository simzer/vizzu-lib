import { CString, CFunction, CEventPtr } from '../cvizzu.types'

import { Anim, Config, Geom, Styles } from '../types/vizzu.js'

import { CObject, CEnv } from './cenv.js'
import { CPointerClosure } from './objregistry.js'
import { CProxy } from './cproxy.js'
import { CCanvas } from './ccanvas.js'
import { CAnimation } from './canimctrl.js'

export class Snapshot extends CObject {}

export class CEvent extends CObject {
  preventDefault(): void {
    this._call(this._wasm._event_preventDefault)()
  }
}

class CConfig extends CProxy<Config.Chart> {}
class CStyle extends CProxy<Styles.Chart> {}
class CAnimOptions extends CProxy<Anim.Options> {}

export class CChart extends CObject {
  config: CConfig
  style: CStyle
  computedStyle: CStyle
  animOptions: CAnimOptions

  constructor(env: CEnv, getId: CPointerClosure) {
    super(getId, env)

    this.config = this._makeConfig()
    this.style = this._makeStyle(false)
    this.computedStyle = this._makeStyle(true)
    this.animOptions = this._makeAnimOptions()
  }

  update(cCanvas: CCanvas, width: number, height: number, renderControl: number): void {
    this._call(this._wasm._vizzu_update)(cCanvas.getId(), width, height, renderControl)
  }

  animate(callback: (ok: boolean) => void): void {
    const callbackPtr = this._wasm.addFunction((ok: boolean) => {
      callback(ok)
      this._wasm.removeFunction(callbackPtr)
    }, 'vi')
    this._call(this._wasm._chart_animate)(callbackPtr)
  }

  storeSnapshot(): Snapshot {
    return new Snapshot(this._get(this._wasm._chart_store), this)
  }

  restoreSnapshot(snapshot: Snapshot): void {
    this._call(this._wasm._chart_restore)(snapshot.getId())
  }

  restoreAnim(animation: CAnimation): void {
    this._call(this._wasm._chart_anim_restore)(animation.getId())
  }

  setKeyframe(): void {
    this._call(this._wasm._chart_setKeyframe)()
  }

  addEventListener<T>(eventName: string, func: (event: CEvent, param: T) => void): CFunction {
    const wrappedFunc = (eventPtr: CEventPtr, param: CString): void => {
      const eventObj = new CEvent(() => eventPtr, this)
      func(eventObj, JSON.parse(this._fromCString(param)))
    }
    const cfunc = this._wasm.addFunction(wrappedFunc, 'vii')
    const cname = this._toCString(eventName)
    try {
      this._call(this._wasm._addEventListener)(cname, cfunc)
    } finally {
      this._wasm._free(cname)
    }
    return cfunc
  }

  removeEventListener(eventName: string, cfunc: CFunction): void {
    const cname = this._toCString(eventName)
    try {
      this._call(this._wasm._removeEventListener)(cname, cfunc)
    } finally {
      this._wasm._free(cname)
    }
    this._wasm.removeFunction(cfunc)
  }

  getMarkerData(markerId: number): unknown {
    const cStr = this._call(this._wasm._chart_markerData)(markerId)
    return JSON.parse(this._fromCString(cStr))
  }

  toCanvasCoords(point: Geom.Point): Geom.Point {
    const ptr = this._call(this._wasm._chart_relToCanvasCoords)(point.x, point.y)
    return {
      x: this._wasm.getValue(ptr, 'double'),
      y: this._wasm.getValue(ptr + 8, 'double')
    }
  }

  toRelCoords(point: Geom.Point): Geom.Point {
    const ptr = this._call(this._wasm._chart_canvasToRelCoords)(point.x, point.y)
    return {
      x: this._wasm.getValue(ptr, 'double'),
      y: this._wasm.getValue(ptr + 8, 'double')
    }
  }

  pointerdown(canvas: CCanvas, pointerId: number, x: number, y: number): void {
    this._call(this._wasm._vizzu_pointerDown)(canvas.getId(), pointerId, x, y)
  }

  pointermove(canvas: CCanvas, pointerId: number, x: number, y: number): void {
    this._call(this._wasm._vizzu_pointerMove)(canvas.getId(), pointerId, x, y)
  }

  pointerup(canvas: CCanvas, pointerId: number, x: number, y: number): void {
    this._call(this._wasm._vizzu_pointerUp)(canvas.getId(), pointerId, x, y)
  }

  pointerleave(canvas: CCanvas, pointerId: number): void {
    this._call(this._wasm._vizzu_pointerLeave)(canvas.getId(), pointerId)
  }

  wheel(canvas: CCanvas, delta: number): void {
    this._call(this._wasm._vizzu_wheel)(canvas.getId(), delta)
  }

  private _makeConfig(): CConfig {
    return new CConfig(
      this.getId,
      this,
      this._wasm._chart_getList,
      this._wasm._chart_getValue,
      this._wasm._chart_setValue
    )
  }

  private _makeStyle(computed: boolean): CStyle {
    return new CStyle(
      this.getId,
      this,
      this._wasm._style_getList,
      (ptr, path) => this._wasm._style_getValue(ptr, path, computed),
      this._wasm._style_setValue
    )
  }

  private _makeAnimOptions(): CAnimOptions {
    return new CAnimOptions(
      this.getId,
      this,
      () => 0,
      () => 0,
      this._wasm._anim_setValue
    )
  }
}
