import { CObject } from './cenv.js'

export class CAnimation extends CObject {}

export class CAnimControl extends CObject {
  animControl(command: string, param = ''): void {
    const ccommand = this._toCString(command)
    const cparam = this._toCString(param)
    try {
      this._call(this._wasm._anim_control)(ccommand, cparam)
    } finally {
      this._wasm._free(cparam)
      this._wasm._free(ccommand)
    }
  }

  storeAnim(): CAnimation {
    return new CAnimation(this._get(this._wasm._chart_anim_store), this)
  }
}
