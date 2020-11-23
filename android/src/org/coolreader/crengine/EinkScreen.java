package org.coolreader.crengine;

import android.content.Context;
import android.util.Log;
import android.view.View;

import com.onyx.android.sdk.api.device.epd.EpdController;
import com.onyx.android.sdk.api.device.epd.UpdateMode;
import com.onyx.android.sdk.device.Device;

import org.coolreader.CoolReader;

import java.util.Arrays;
import java.util.List;

public class EinkScreen {
	private static final String TAG = "EinkScreen";
	/// variables
	private static int mUpdateMode = -1;
	// 0 - CLEAR_ALL, set only for old_mode == 2
	// 1 - ONESHOT, always set in prepare
	// 2 - ACTIVE, set in prepare
	private static int mUpdateInterval;
	private static int mRefreshNumber = -1;
	private static boolean mIsSleep = false;
	private static boolean mIsSupportRegal = false;
	private static boolean mInFastMode = false;
	private static boolean mInA2Mode = false;
	// constants
	public final static int CMODE_CLEAR = 0;
	public final static int CMODE_ONESHOT = 1;
	public final static int CMODE_ACTIVE = 2;
	// Front light levels
	private static List<Integer> mFrontLineLevels = null;
	private static List<Integer> mWarmLightLevels = null;

	public static void Refresh() {
		mRefreshNumber = -1;
	}

	public static void PrepareController(View view, boolean isPartially) {
		if (DeviceInfo.EINK_NOOK || DeviceInfo.EINK_TOLINO) {
			//System.err.println("Sleep = " + isPartially);
			if (isPartially || mIsSleep != isPartially) {
				SleepController(isPartially, view);
//				if (isPartially) 
					return;
			}
			if (mRefreshNumber == -1) {
				switch (mUpdateMode) {
					case CMODE_CLEAR:
						SetMode(view, mUpdateMode);
						break;
					case CMODE_ACTIVE:
						if (mUpdateInterval == 0) {
							SetMode(view, mUpdateMode);
						}
						break;
				}
				mRefreshNumber = 0;
				return;
			}
			if (mUpdateMode == CMODE_CLEAR) {
				SetMode(view, CMODE_CLEAR);
				return;
			}
			if (mUpdateInterval > 0 || mUpdateMode == CMODE_ONESHOT) {
				if (mRefreshNumber == 0 || (mUpdateMode == CMODE_ONESHOT && mRefreshNumber < mUpdateInterval)) {
					switch (mUpdateMode) {
						case CMODE_ACTIVE:
							SetMode(view, CMODE_ACTIVE);
							break;
						case CMODE_ONESHOT:
							SetMode(view, CMODE_ONESHOT);
							break;
					}
				} else if (mUpdateInterval <= mRefreshNumber) {
					SetMode(view, CMODE_CLEAR);
					mRefreshNumber = -1;
				}
				if (mUpdateInterval > 0) {
					mRefreshNumber++;
				}
			}
			/*
			if (mUpdateMode == 1 && mUpdateInterval != 0) {
				if (mRefreshNumber == 0) {
					// быстрый режим, один раз устанавливается
					N2EpdController.setMode(N2EpdController.REGION_APP_3,
											N2EpdController.WAVE_GL16,
											N2EpdController.MODE_ACTIVE, view); // why not MODE_ACTIVE_ALL?
				} else if (mUpdateInterval == mRefreshNumber) {
					// одно качественное обновление для быстрого режима
					N2EpdController.setMode(N2EpdController.REGION_APP_3,
											N2EpdController.WAVE_GU,
											N2EpdController.MODE_CLEAR_ALL, view);
					mRefreshNumber = -1;
				}
				mRefreshNumber ++;
			}
			*/
		} else if (DeviceInfo.EINK_ONYX) {
			if (mRefreshNumber == -1) {
				mRefreshNumber = 0;
				onyxRepaintEveryThing(view);
				return;
			}
			if (mUpdateInterval > 0) {
				mRefreshNumber++;
				if (mRefreshNumber >= mUpdateInterval) {
					mRefreshNumber = 0;
					onyxRepaintEveryThing(view);
				}
			}
		}
	}

	public static void UpdateController(View view, boolean isPartially) {
		if (DeviceInfo.EINK_ONYX) {
			if (mRefreshNumber > 0 || mUpdateInterval == 0) {
				switch (mUpdateMode) {
					case CMODE_CLEAR:
						EpdController.setViewDefaultUpdateMode(view, mIsSupportRegal ? UpdateMode.REGAL : UpdateMode.GU);
						break;
					case CMODE_ONESHOT:
						EpdController.setViewDefaultUpdateMode(view, UpdateMode.DU);
						break;
					default:
				}
				// I don't know what exactly this line does, but without it, the image on rk3288 will not updated.
				// Found by brute force.
				EpdController.byPass(0);
			}
		}
	}

	private static void onyxRepaintEveryThing(View view) {
		switch (Device.currentDeviceIndex) {
			case Rk31xx:
			case Rk32xx:
			case Rk33xx:
			case SDM:
				EpdController.repaintEveryThing(UpdateMode.GC);
				break;
			default:
				EpdController.setViewDefaultUpdateMode(view, UpdateMode.GC);
				break;
		}
	}

	public static void ResetController(int mode, int updateInterval, View view) {
		mUpdateInterval = updateInterval;
		ResetController(mode, view);
	}

	public static void ResetController(int mode, View view) {
		if (DeviceInfo.EINK_NOOK || DeviceInfo.EINK_TOLINO) {
			System.err.println("+++ResetController " + mode);
			switch (mode) {
				case CMODE_CLEAR:
					if (mUpdateMode == CMODE_ACTIVE) {
						mRefreshNumber = -1;
					} else {
						mRefreshNumber = 0;
					}
					break;
				case CMODE_ONESHOT:
					mRefreshNumber = 0;
					break;
				default:
					mRefreshNumber = -1;
			}
		} else if (DeviceInfo.EINK_ONYX) {
			EpdController.enableScreenUpdate(view, true);
			mIsSupportRegal = EpdController.supportRegal();
			mRefreshNumber = 0;
			if (mUpdateInterval == 0)
				onyxRepaintEveryThing(view);
			EpdController.clearApplicationFastMode();
			switch (mode) {
				case CMODE_CLEAR:			// Quality
					if (mInA2Mode) {
						EpdController.disableA2ForSpecificView(view);
						mInA2Mode = false;
					}
					if (mInFastMode) {
						EpdController.applyApplicationFastMode(CoolReader.class.getSimpleName(), false, true);
						mInFastMode = false;
					}
					break;
				case CMODE_ONESHOT:			// Fast
					if (mInA2Mode) {
						EpdController.disableA2ForSpecificView(view);
						mInA2Mode = false;
					}
					// Enable fast mode (not implemented on RK3026, not tested)
					if (!mInFastMode) {
						EpdController.applyApplicationFastMode(CoolReader.class.getSimpleName(), true, true);
						mInFastMode = true;
					}
					break;
				case CMODE_ACTIVE:			// Fast 2 (A2 mode)
					if (mInFastMode) {
						EpdController.applyApplicationFastMode(CoolReader.class.getSimpleName(), false, true);
						mInFastMode = false;
					}
					if (!mInA2Mode) {
						EpdController.enableA2ForSpecificView(view);
						mInA2Mode = true;
					}
					break;
			}
			Log.d(TAG, "EinkScreen: Regal is " + (mIsSupportRegal ? "" : "NOT ") + "supported");
		}
		mUpdateMode = mode;
	}

	public static void ResetController(View view) {
		if (DeviceInfo.EINK_NOOK || DeviceInfo.EINK_TOLINO) {
			if (mUpdateInterval != CMODE_CLEAR) {
				System.err.println("+++Soft reset Controller ");
				SetMode(view, CMODE_CLEAR);
				mRefreshNumber = -1;
			}
		}
	}

	private static void SleepController(boolean toSleep, View view) {
		if (DeviceInfo.EINK_NOOK || DeviceInfo.EINK_TOLINO) {
			if (toSleep != mIsSleep) {
				System.err.println("+++SleepController " + toSleep);
				mIsSleep = toSleep;
				if (mIsSleep) {
					switch (mUpdateMode) {
						case CMODE_CLEAR:
							break;
						case CMODE_ONESHOT:
							break;
						case CMODE_ACTIVE:
							SetMode(view, CMODE_CLEAR);
							mRefreshNumber = -1;
					}
				} else {
					ResetController(mUpdateMode, view);
				}
			}
		}
	}

	private static void SetMode(View view, int mode) {
		if (DeviceInfo.EINK_TOLINO) {
			TolinoEpdController.setMode(view, mode);
		} else if (DeviceInfo.EINK_NOOK) {
			switch (mode) {
				case CMODE_CLEAR:
					N2EpdController.setMode(N2EpdController.REGION_APP_3,
							N2EpdController.WAVE_GC,
							N2EpdController.MODE_ONESHOT_ALL);
//					N2EpdController.MODE_CLEAR, view);
					break;
				case CMODE_ONESHOT:
					N2EpdController.setMode(N2EpdController.REGION_APP_3,
							N2EpdController.WAVE_GU,
							N2EpdController.MODE_ONESHOT_ALL);
//					N2EpdController.MODE_ONESHOT_ALL, view);
					break;
				case CMODE_ACTIVE:
					N2EpdController.setMode(N2EpdController.REGION_APP_3,
							N2EpdController.WAVE_GL16,
							N2EpdController.MODE_ACTIVE_ALL);
//					N2EpdController.MODE_ACTIVE_ALL, view);
					break;
			}
		}
	}

	public static int getUpdateMode() {
		return mUpdateMode;
	}

	public static int getUpdateInterval() {
		return mUpdateInterval;
	}

	public static boolean setFrontLightValue(Context context, int value) {
		boolean res = false;
		if (DeviceInfo.EINK_ONYX) {
			if (DeviceInfo.ONYX_HAVE_FRONTLIGHT) {
				if (value >= 0) {
					Integer alignedValue = Utils.findNearestValue(getFrontLightLevels(context), value);
					if (null != alignedValue) {
						if (DeviceInfo.ONYX_HAVE_NATURAL_BACKLIGHT) {
							res = Device.currentDevice().setColdLightDeviceValue(context, alignedValue);
						} else {
							if (Device.currentDevice().setFrontLightDeviceValue(context, alignedValue))
								res = Device.currentDevice().setFrontLightConfigValue(context, alignedValue);
						}
					}
				} else {
					// system default, just ignore
				}
			}
		}
		return res;
	}

	public static boolean setWarmLightValue(Context context, int value) {
		boolean res = false;
		if (DeviceInfo.EINK_ONYX) {
			if (DeviceInfo.ONYX_HAVE_NATURAL_BACKLIGHT) {
				if (value >= 0) {
					Integer alignedValue = Utils.findNearestValue(getWarmLightLevels(context), value);
					if (null != alignedValue) {
						res = Device.currentDevice().setWarmLightDeviceValue(context, alignedValue);
					}
				} else {
					// system default, just ignore
				}
			}
		}
		return res;
	}

	public static List<Integer> getFrontLightLevels(Context context) {
		if (null == mFrontLineLevels) {
			if (DeviceInfo.EINK_HAVE_FRONTLIGHT) {
				if (DeviceInfo.ONYX_HAVE_FRONTLIGHT) {
					mFrontLineLevels = Device.currentDevice().getFrontLightValueList(context);
					if (null == mFrontLineLevels) {
						Integer[] values = Device.currentDevice().getColdLightValues(context);
						if (null != values) {
							mFrontLineLevels = Arrays.asList(values);
						}
					}
				}
				// TODO: other e-ink devices with front light support...
			}
		}
		return mFrontLineLevels;
	}

	public static List<Integer> getWarmLightLevels(Context context) {
		if (null == mWarmLightLevels) {
			if (DeviceInfo.EINK_HAVE_NATURAL_BACKLIGHT) {
				if (DeviceInfo.ONYX_HAVE_NATURAL_BACKLIGHT) {
					Integer[] values = Device.currentDevice().getWarmLightValues(context);
					if (null != values) {
						mWarmLightLevels = Arrays.asList(values);
					}
				}
				// TODO: other e-ink devices with front light support...
			}
		}
		return mWarmLightLevels;
	}
}
