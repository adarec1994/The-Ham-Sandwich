using ProjectWS.FileFormats.Common;

namespace ProjectWS.FileFormats.M3
{
    public partial class Submesh : ArrayData
    {
        public enum BodyPart
        {
            BODY_None = 0,
            BODY_LeftAnkle = 1,
            BODY_RightAnkle = 2,
            BODY_LeftUpperArm = 3,
            BODY_RightUpperArm = 4,
            BODY_LeftLowerLeg = 5,
            BODY_RightLowerLeg = 6,
            BODY_LeftKneeBelow = 7,
            BODY_RightKneeBelow = 8,
            BODY_LowerChestAndBack = 9,
            BODY_ShouldersAndBack = 10,
            BODY_LeftEar = 11,
            BODY_RightEar = 12,
            BODY_LeftElbow = 13,
            BODY_RightElbow = 14,
            BODY_FaceElement = 15,          // Used for eyes, faces, piercings and sometimes for other facial elements 
            BODY_LeftFingers = 16,
            BODY_RightFingers = 17,
            BODY_LeftHeel = 18,
            BODY_RightHeel = 19,
            BODY_LeftLowerArm = 20,
            BODY_RightLowerArm = 21,
            BODY_LeftPalm = 22,
            BODY_RightPalm = 23,
            BODY_LeftKnee = 24,
            BODY_RightKnee = 25,
            BODY_Neck = 26,
            BODY_Pelvis = 27,
            BODY_LeftArmTriceps = 29,
            BODY_RightArmTriceps = 30,
            BODY_LeftThigh = 33,
            BODY_RightThigh = 34,
            BODY_LeftToes = 35,
            BODY_RightToes = 36,
            BODY_MidRiff = 37,
            BODY_LeftWrist = 38,
            BODY_RightWrist = 39,
            BODY_Head = 81,
            BODY_LeftThighOrKneeFur = 82,    // This seems exclusive to chua 
            BODY_RightThighOrKneeFur = 83,   // Can be seen on male granok and human, kind of placeholder for the face, but inside of the head 
            BODY_InnerHead = 84,
            BODY_Chest = 85,
            BODY_Tail = 88,
            BODY_UpperBust = 91,             // May be female specific
            BODY_NeckStart = 92,
            BODY_Horns = 96
        }
    }
}