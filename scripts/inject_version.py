Import("env")

import os

version = os.environ.get("FIRMWARE_VERSION")
if version:
    if len(version) > 31 or not all(
        char.isalnum() or char in "._+-" for char in version
    ):
        raise ValueError("FIRMWARE_VERSION contains unsupported characters")

    macro = (
        "TILT_FIRMWARE_VERSION"
        if env.get("PIOENV") == "tilt_platform"
        else "FIRMWARE_VERSION"
    )
    env.Append(CPPDEFINES=[(macro, '\\"{}\\"'.format(version))])
