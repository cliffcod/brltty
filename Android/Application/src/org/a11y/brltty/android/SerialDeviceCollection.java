/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2018 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.com/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

package org.a11y.brltty.android;

import android.content.Context;

public final class SerialDeviceCollection extends DeviceCollection {
  public static final String DEVICE_QUALIFIER = "serial";

  @Override
  public String getQualifier () {
    return DEVICE_QUALIFIER;
  }

  public SerialDeviceCollection (Context context) {
    super();
  }

  @Override
  public String[] getIdentifiers () {
    return new String[0];
  }

  @Override
  public String[] getLabels () {
    return new String[0];
  }

  @Override
  public String makeReference (String identifier) {
    return identifier;
  }
}