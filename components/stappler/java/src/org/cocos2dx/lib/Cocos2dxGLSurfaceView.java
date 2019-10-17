/****************************************************************************
Copyright (c) 2010-2011 cocos2d-x.org

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
 ****************************************************************************/
package org.cocos2dx.lib;

import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Rect;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.Handler;
import android.os.Message;
import android.text.InputType;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.TextView;

public class Cocos2dxGLSurfaceView extends GLSurfaceView {
	// ===========================================================
	// Constants
	// ===========================================================

	private static final String TAG = Cocos2dxGLSurfaceView.class.getSimpleName();

	private final static int HANDLER_OPEN_IME_KEYBOARD = 2;
	private final static int HANDLER_CLOSE_IME_KEYBOARD = 3;

	// ===========================================================
	// Fields
	// ===========================================================

	// TODO Static handler -> Potential leak!
	private static Handler sHandler;

	private static Cocos2dxGLSurfaceView mCocos2dxGLSurfaceView;
	private static Cocos2dxTextInputWraper sCocos2dxTextInputWraper;

	private Cocos2dxRenderer mCocos2dxRenderer;
	private Cocos2dxEditText mCocos2dxEditText;

	// ===========================================================
	// Constructors
	// ===========================================================

	public Cocos2dxGLSurfaceView(final Context context) {
		super(context);

		this.initView();
	}

	public Cocos2dxGLSurfaceView(final Context context, final AttributeSet attrs) {
		super(context, attrs);
		
		this.initView();
	}

	private boolean isFullScreenEdit() {
		final TextView textField = mCocos2dxEditText;
		final InputMethodManager imm = (InputMethodManager) textField.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
		return imm.isFullscreenMode();
	}

	protected boolean _keyboardEnabled = false;
	protected boolean _inputEnabled = false;
	protected Rect _clipRect = new Rect();

	protected void setKeyboardEnabled(boolean value) {
		_keyboardEnabled = value;
	}

	protected void onLayout(boolean changed, int left, int top, int right, int bottom) {
		super.onLayout(changed, left, top, right, bottom);

		if (isFullScreenEdit()) {
			if (_keyboardEnabled) {
				onKeyboardNotification(false, getWidth(), 0);
				setKeyboardEnabled(false);
			}
			return;
		}

		int statusBar = 0;
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
			int resourceId = getContext().getResources().getIdentifier("status_bar_height", "dimen", "android");
			if (resourceId > 0) {
				statusBar = getContext().getResources().getDimensionPixelSize(resourceId);
			}
		}

    	this.getWindowVisibleDisplayFrame(_clipRect);
    	int height = getHeight() - (_clipRect.bottom - _clipRect.top);
    	
    	InputMethodManager imm = (InputMethodManager) getContext()
                .getSystemService(Context.INPUT_METHOD_SERVICE);

		Configuration cfg = getContext().getResources().getConfiguration();
		if (cfg.keyboard != Configuration.KEYBOARD_QWERTY) {
			if (imm.isActive(mCocos2dxEditText)) {
				if (height <= statusBar) {
					mCocos2dxEditText.removeTextChangedListener(sCocos2dxTextInputWraper);
					imm.hideSoftInputFromWindow(mCocos2dxEditText.getWindowToken(), 0);
					requestFocus();

					if (_keyboardEnabled) {
						onKeyboardNotification(false, getWidth(), height);
						setKeyboardEnabled(false);
						finalizeInput();
					}
				} else {
					setKeyboardEnabled(true);
					onKeyboardNotification(true, getWidth(), height - statusBar);
					if (!_inputEnabled) {
						onInputEnabled(true);
						_inputEnabled = true;
					}
				}
			} else {
				if (_keyboardEnabled) {
					onKeyboardNotification(false, getWidth(), height);
					setKeyboardEnabled(true);
					finalizeInput();
				}
			}
        }
	}
	native void onKeyboardNotification(boolean enabled, float width, float height);
	
	protected void initView() {
		this.setEGLContextClientVersion(2);
		this.setFocusableInTouchMode(true);

		Cocos2dxGLSurfaceView.mCocos2dxGLSurfaceView = this;
		Cocos2dxGLSurfaceView.sCocos2dxTextInputWraper = new Cocos2dxTextInputWraper(this);
	}

	// ===========================================================
	// Getter & Setter
	// ===========================================================

	private final static int InputType_Date_Date = 1;
	private final static int InputType_Date_DateTime = 2;
	private final static int InputType_Date_Time = 3;

	private final static int InputType_Number_Numbers = 4;
	private final static int InputType_Number_Decimial = 5;
	private final static int InputType_Number_Signed = 6;

	private final static int InputType_Phone = 7;

	private final static int InputType_Text_Text = 8;
	private final static int InputType_Text_Search = 9;
	private final static int InputType_Text_Punctuation = 10;
	private final static int InputType_Text_Email = 11;
	private final static int InputType_Text_Url = 12;

	private final static int InputType_ClassMask = 0x1F;
	private final static int InputType_PasswordBit = 0x20;
	private final static int InputType_MultiLineBit = 0x40;
	private final static int InputType_AutoCorrectionBit = 0x80;

	private int getInputTypeFromNative(int nativeValue) {
		int cl = nativeValue & InputType_ClassMask;
		int ret = InputType.TYPE_CLASS_TEXT;
		if (cl == InputType_Date_Date || cl == InputType_Date_DateTime || cl == InputType_Date_Time) {
			ret = InputType.TYPE_CLASS_DATETIME;
			if (cl == InputType_Date_Date) {
				ret |= InputType.TYPE_DATETIME_VARIATION_DATE;
			} else if (cl == InputType_Date_DateTime) {
				ret |= InputType.TYPE_DATETIME_VARIATION_NORMAL;
			} else if (cl == InputType_Date_Time) {
				ret |= InputType.TYPE_DATETIME_VARIATION_TIME;
			}
		} else if (cl == InputType_Number_Numbers || cl == InputType_Number_Decimial || cl == InputType_Number_Signed) {
			ret = InputType.TYPE_CLASS_NUMBER;
			if (cl == InputType_Number_Decimial) {
				ret |= InputType.TYPE_NUMBER_FLAG_DECIMAL;
			} else if (cl == InputType_Number_Signed) {
				ret |= InputType.TYPE_NUMBER_FLAG_SIGNED;
			}
			if ((nativeValue & InputType_PasswordBit) != 0) {
				ret |= InputType.TYPE_NUMBER_VARIATION_PASSWORD;
			} else {
				ret |= InputType.TYPE_NUMBER_VARIATION_NORMAL;
			}
		} else if (cl == InputType_Phone) {
			ret = InputType.TYPE_CLASS_PHONE;
		} else {
			ret = InputType.TYPE_CLASS_TEXT;
			if ((nativeValue & InputType_PasswordBit) != 0) {
				ret |= InputType.TYPE_TEXT_VARIATION_PASSWORD;
			} else if (cl == InputType_Text_Text || cl == InputType_Text_Punctuation) {
				ret |= InputType.TYPE_TEXT_VARIATION_NORMAL;
			} else if (cl == InputType_Text_Search) {
				ret |= InputType.TYPE_TEXT_VARIATION_WEB_EDIT_TEXT;
			} else if (cl == InputType_Text_Email) {
				ret |= InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS;
			} else if (cl == InputType_Text_Url) {
				ret |= InputType.TYPE_TEXT_VARIATION_URI;
			}

			if ((nativeValue & InputType_PasswordBit) == 0) {
				if ((nativeValue & InputType_MultiLineBit) != 0) {
					ret |= InputType.TYPE_TEXT_FLAG_MULTI_LINE;
				}
				if ((nativeValue & InputType_AutoCorrectionBit) != 0) {
					ret |= InputType.TYPE_TEXT_FLAG_AUTO_CORRECT;
				} else {
					ret |= InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS;
				}
			}
		}
		return ret;
	}
	
	protected boolean _protectedEnvironment = false;
	protected boolean _shouldUpdateString = false;

	public void runInput(String text, int cursorStart, int cursorLen, int type) {
		if (mCocos2dxEditText != null) {
			_protectedEnvironment = true;
			if (!isFullScreenEdit()) {
				_inputEnabled = true;
				onInputEnabled(true);
			}
			mCocos2dxEditText.removeTextChangedListener(Cocos2dxGLSurfaceView.sCocos2dxTextInputWraper);
			mCocos2dxEditText.setText(text);
			mCocos2dxEditText.setInputType(getInputTypeFromNative(type));
			if (cursorStart > text.length()) {
				cursorStart = text.length();
			}
			if (cursorLen == 0) {
				mCocos2dxEditText.setSelection(cursorStart);
			} else {
				mCocos2dxEditText.setSelection(cursorStart, cursorStart + cursorLen);
			}
			Cocos2dxGLSurfaceView.sCocos2dxTextInputWraper.setOriginText(text);
			mCocos2dxEditText.addTextChangedListener(Cocos2dxGLSurfaceView.sCocos2dxTextInputWraper);
			mCocos2dxEditText.requestFocus();
			
			final InputMethodManager imm = (InputMethodManager) getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
			imm.showSoftInput(mCocos2dxEditText, 0);
			_protectedEnvironment = false;
			if (_shouldUpdateString) {
				onTextChanged(mCocos2dxEditText.getText().toString());
			}
		}
	}
	
	public void updateInput(String text, int cursorStart, int cursorLen, int type) {
		if (mCocos2dxEditText != null) {
			_protectedEnvironment = true;
			mCocos2dxEditText.setInputType(getInputTypeFromNative(type));
			mCocos2dxEditText.setText(text);
			if (text.isEmpty()) {
				mCocos2dxEditText.setSelection(0);
			} else if (cursorLen == 0) {
				mCocos2dxEditText.setSelection(cursorStart);
			} else {
				mCocos2dxEditText.setSelection(cursorStart, cursorStart + cursorLen);
			}
			_protectedEnvironment = false;
			if (_shouldUpdateString) {
				onTextChanged(mCocos2dxEditText.getText().toString());
			}
		}
	}

	public void updateCursor(int cursorStart, int cursorLen) {
		if (mCocos2dxEditText != null) {
			_protectedEnvironment = true;
			if (cursorLen == 0) {
				mCocos2dxEditText.setSelection(cursorStart);
			} else {
				mCocos2dxEditText.setSelection(cursorStart, cursorStart	+ cursorLen);
			}
			_protectedEnvironment = false;
			if (_shouldUpdateString) {
				onTextChanged(mCocos2dxEditText.getText().toString());
			}
		}
	}
	
	public void cancelInput() {
		if (mCocos2dxEditText != null) {
			mCocos2dxEditText.removeTextChangedListener(Cocos2dxGLSurfaceView.sCocos2dxTextInputWraper);
			final InputMethodManager imm = (InputMethodManager) getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
			imm.hideSoftInputFromWindow(mCocos2dxEditText.getWindowToken(), 0);
			requestFocus();
		}
	}

	public void finalizeInput() {
		if (_inputEnabled) {
			_inputEnabled = false;
			onInputEnabled(false);
		}
		onCancelInput();
	}
	protected native void onCancelInput();

	public void onTextChanged(final String pText) {
		if (!_protectedEnvironment) {
			boolean isFullscreen = isFullScreenEdit();
			if (isFullscreen) {
				onInputEnabled(true);
			}
			final int start = this.mCocos2dxEditText.getSelectionStart();
			final int end = this.mCocos2dxEditText.getSelectionEnd();
			nativeTextChanged(pText, start, end - start);
			if (isFullscreen) {
				onInputEnabled(false);
			}
			_shouldUpdateString = false;
		} else {
			_shouldUpdateString = true;
		}
	}

	native public void nativeTextChanged(String text, int cursorStart, int cursorLen);
	native public void onInputEnabled(boolean value);
	
	public static Cocos2dxGLSurfaceView getInstance() {
		return mCocos2dxGLSurfaceView;
	}

	public static void queueAccelerometer(final float x, final float y, final float z, final long timestamp) {

	}

	public void setCocos2dxRenderer(final Cocos2dxRenderer renderer) {
		this.mCocos2dxRenderer = renderer;
		this.setRenderer(this.mCocos2dxRenderer);
	}

	private String getContentText() {
		return this.mCocos2dxRenderer.getContentText();
	}

	public Cocos2dxEditText getCocos2dxEditText() {
		return this.mCocos2dxEditText;
	}

	public void setCocos2dxEditText(final Cocos2dxEditText pCocos2dxEditText) {
		this.mCocos2dxEditText = pCocos2dxEditText;
		if (null != this.mCocos2dxEditText && null != Cocos2dxGLSurfaceView.sCocos2dxTextInputWraper) {
			this.mCocos2dxEditText.setImeOptions(EditorInfo.IME_FLAG_NO_EXTRACT_UI);
			this.mCocos2dxEditText.setOnEditorActionListener(Cocos2dxGLSurfaceView.sCocos2dxTextInputWraper);
			this.mCocos2dxEditText.setCocos2dxGLSurfaceView(this);
			this.requestFocus();
		}
	}

	// ===========================================================
	// Methods for/from SuperClass/Interfaces
	// ===========================================================

	@Override
	public void onResume() {
		super.onResume();
		this.setRenderMode(RENDERMODE_WHEN_DIRTY);
		this.queueEvent(new Runnable() {
			@Override
			public void run() {
				Cocos2dxGLSurfaceView.this.mCocos2dxRenderer.handleOnResume();
			}
		});
	}

	@Override
	public void onPause() {
		this.queueEvent(new Runnable() {
			@Override
			public void run() {
				Cocos2dxGLSurfaceView.this.mCocos2dxRenderer.handleOnPause();
			}
		});
		this.setRenderMode(RENDERMODE_WHEN_DIRTY);
		//super.onPause();
	}

	@Override
	public boolean onTouchEvent(final MotionEvent pMotionEvent) {
		// these data are used in ACTION_MOVE and ACTION_CANCEL
		final int pointerNumber = pMotionEvent.getPointerCount();
		final int[] ids = new int[pointerNumber];
		final float[] xs = new float[pointerNumber];
		final float[] ys = new float[pointerNumber];

		for (int i = 0; i < pointerNumber; i++) {
			ids[i] = pMotionEvent.getPointerId(i);
			xs[i] = pMotionEvent.getX(i);
			ys[i] = pMotionEvent.getY(i);
		}

		switch (pMotionEvent.getAction() & MotionEvent.ACTION_MASK) {
			case MotionEvent.ACTION_POINTER_DOWN:
				final int indexPointerDown = pMotionEvent.getAction() >> MotionEvent.ACTION_POINTER_INDEX_SHIFT;
				final int idPointerDown = pMotionEvent.getPointerId(indexPointerDown);
				final float xPointerDown = pMotionEvent.getX(indexPointerDown);
				final float yPointerDown = pMotionEvent.getY(indexPointerDown);

				this.queueEvent(new Runnable() {
					@Override
					public void run() {
						Cocos2dxGLSurfaceView.this.mCocos2dxRenderer.handleActionDown(idPointerDown, xPointerDown, yPointerDown);
					}
				});
				break;

			case MotionEvent.ACTION_DOWN:
				// there are only one finger on the screen
				final int idDown = pMotionEvent.getPointerId(0);
				final float xDown = xs[0];
				final float yDown = ys[0];

				this.queueEvent(new Runnable() {
					@Override
					public void run() {
						Cocos2dxGLSurfaceView.this.mCocos2dxRenderer.handleActionDown(idDown, xDown, yDown);
					}
				});
				break;

			case MotionEvent.ACTION_MOVE:
				this.queueEvent(new Runnable() {
					@Override
					public void run() {
						Cocos2dxGLSurfaceView.this.mCocos2dxRenderer.handleActionMove(ids, xs, ys);
					}
				});
				break;

			case MotionEvent.ACTION_POINTER_UP:
				final int indexPointUp = pMotionEvent.getAction() >> MotionEvent.ACTION_POINTER_INDEX_SHIFT;
				final int idPointerUp = pMotionEvent.getPointerId(indexPointUp);
				final float xPointerUp = pMotionEvent.getX(indexPointUp);
				final float yPointerUp = pMotionEvent.getY(indexPointUp);

				this.queueEvent(new Runnable() {
					@Override
					public void run() {
						Cocos2dxGLSurfaceView.this.mCocos2dxRenderer.handleActionUp(idPointerUp, xPointerUp, yPointerUp);
					}
				});
				break;

			case MotionEvent.ACTION_UP:
				// there are only one finger on the screen
				final int idUp = pMotionEvent.getPointerId(0);
				final float xUp = xs[0];
				final float yUp = ys[0];

				this.queueEvent(new Runnable() {
					@Override
					public void run() {
						Cocos2dxGLSurfaceView.this.mCocos2dxRenderer.handleActionUp(idUp, xUp, yUp);
					}
				});
				break;

			case MotionEvent.ACTION_CANCEL:
				this.queueEvent(new Runnable() {
					@Override
					public void run() {
						Cocos2dxGLSurfaceView.this.mCocos2dxRenderer.handleActionCancel(ids, xs, ys);
					}
				});
				break;
		}

        /*
		if (BuildConfig.DEBUG) {
			Cocos2dxGLSurfaceView.dumpMotionEvent(pMotionEvent);
		}
		*/
		return true;
	}

	/*
	 * This function is called before Cocos2dxRenderer.nativeInit(), so the
	 * width and height is correct.
	 */
	@Override
	protected void onSizeChanged(final int pNewSurfaceWidth, final int pNewSurfaceHeight, final int pOldSurfaceWidth, final int pOldSurfaceHeight) {
		if(!this.isInEditMode()) {
			this.mCocos2dxRenderer.setScreenWidthAndHeight(pNewSurfaceWidth, pNewSurfaceHeight);
		}
	}

	@Override
	public boolean onKeyDown(final int pKeyCode, final KeyEvent pKeyEvent) {
		switch (pKeyCode) {
			case KeyEvent.KEYCODE_BACK:
			case KeyEvent.KEYCODE_MENU:
			case KeyEvent.KEYCODE_DPAD_LEFT:
			case KeyEvent.KEYCODE_DPAD_RIGHT:
			case KeyEvent.KEYCODE_DPAD_UP:
			case KeyEvent.KEYCODE_DPAD_DOWN:
			case KeyEvent.KEYCODE_ENTER:
			case KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE:
			case KeyEvent.KEYCODE_DPAD_CENTER:
				this.queueEvent(new Runnable() {
					@Override
					public void run() {
						Cocos2dxGLSurfaceView.this.mCocos2dxRenderer.handleKeyDown(pKeyCode);
					}
				});
				return true;
			default:
				return super.onKeyDown(pKeyCode, pKeyEvent);
		}
	}

	// ===========================================================
	// Methods
	// ===========================================================

	// ===========================================================
	// Inner and Anonymous Classes
	// ===========================================================

	public static void openIMEKeyboard() { }

	public static void closeIMEKeyboard() { }

	public void insertText(final String pText) {
		this.queueEvent(new Runnable() {
			@Override
			public void run() {
				Cocos2dxGLSurfaceView.this.mCocos2dxRenderer.handleInsertText(pText);
			}
		});
	}

	public void deleteBackward() {
		this.queueEvent(new Runnable() {
			@Override
			public void run() {
				Cocos2dxGLSurfaceView.this.mCocos2dxRenderer.handleDeleteBackward();
			}
		});
	}

	private static void dumpMotionEvent(final MotionEvent event) {
		final String names[] = { "DOWN", "UP", "MOVE", "CANCEL", "OUTSIDE", "POINTER_DOWN", "POINTER_UP", "7?", "8?", "9?" };
		final StringBuilder sb = new StringBuilder();
		final int action = event.getAction();
		final int actionCode = action & MotionEvent.ACTION_MASK;
		sb.append("event ACTION_").append(names[actionCode]);
		if (actionCode == MotionEvent.ACTION_POINTER_DOWN || actionCode == MotionEvent.ACTION_POINTER_UP) {
			sb.append("(pid ").append(action >> MotionEvent.ACTION_POINTER_INDEX_SHIFT);
			sb.append(")");
		}
		sb.append("[");
		for (int i = 0; i < event.getPointerCount(); i++) {
			sb.append("#").append(i);
			sb.append("(pid ").append(event.getPointerId(i));
			sb.append(")=").append((int) event.getX(i));
			sb.append(",").append((int) event.getY(i));
			if (i + 1 < event.getPointerCount()) {
				sb.append(";");
			}
		}
		sb.append("]");
		Log.d(Cocos2dxGLSurfaceView.TAG, sb.toString());
	}
	
	public static void staticRenderFrame() {
		if (mCocos2dxGLSurfaceView != null) {
			if (mCocos2dxGLSurfaceView.getRenderMode() == RENDERMODE_WHEN_DIRTY) {
				mCocos2dxGLSurfaceView.requestRender();
			}
		}
	}
	
	public static void staticSetRenderContinuously(boolean value) {
		if (mCocos2dxGLSurfaceView != null) {
			int mode = mCocos2dxGLSurfaceView.getRenderMode();
			if (value && mode != RENDERMODE_CONTINUOUSLY) {
				mCocos2dxGLSurfaceView.setRenderMode(RENDERMODE_CONTINUOUSLY);
			} else if (!value && mode != RENDERMODE_WHEN_DIRTY) {
				mCocos2dxGLSurfaceView.setRenderMode(RENDERMODE_WHEN_DIRTY);
			}
		}
	}
}
