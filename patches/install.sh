echo $1
rootdirectory="$PWD"
# ---------------------------------

dirs="bionic hardware/interfaces hardware/ril frameworks/av system/core"

for dir in $dirs ; do
	cd $rootdirectory
	cd $dir
	echo "Applying $dir patches..."
	git apply $rootdirectory/device/leeco/x3/patches/$dir/*.patch
	echo " "
done

# -----------------------------------
echo "Changing to build directory..."
cd $rootdirectory

########## frameworks/opt/tele* ##########
cd frameworks/opt/tele*
git remote add vk https://github.com/vishalk95/android_frameworks_opt_telephony_mtk
git fetch vk
git cherry-pick 0dad14f3125859db6bcbdb6c17aa03c0088213ad