/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  Jarno van der Kolk <jarno@jarno.ca>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef PAPAFILE_H
#define PAPAFILE_H

struct PapaFile;
struct PapaModelInformation;
struct PapaTextureInformation;
struct PapaMeshBinding;
struct PapaBoneMapping;
struct PapaMeshInformation;
struct PapaMaterialGroup;
struct PapaMaterialInformation;
struct PapaMaterialVectorParameter;
struct PapaMatrix3;
struct PapaMatrix4;
struct PapaMatrixRow3;
struct PapaMatrixRow4;
struct PapaVerticesInformation;
struct PapaVertex7;
struct PapaVertex8;
struct PapaVertex10;
struct PapaIndicesInformation;
struct PapaPrimitive0;
struct PapaBonesHeader;
struct PapaSkeletonInformation;
struct PapaSkeletonSegment;
struct PapaBone;

struct PapaMatrix4;

struct PapaFileHeader
{
    char Identification[4];
    short Unknown1[2];
    short NumberOfBones;
    short NumberOfTextures;
    short NumberOfVertexBuffers;
    short NumberOfIndexBuffers;
    short NumberOfMaterials;
    short NumberOfMeshes;
    short NumberOfSkeletons;
    short NumberOfModels;
    short Unknown2[4];
    qint64 OffsetBonesHeader;
    qint64 OffsetTextureInformation;
    qint64 OffsetVerticesInformation;
    qint64 OffsetIndicesInformation;
    qint64 OffsetMaterialInformation;
    qint64 OffsetMeshInformation;
    qint64 OffsetSkeletonInformation;
//    local qint64 _OffsetSkeletonHeader = OffsetSkeletonInformation;
    qint64 OffsetModelInformation;
    qint64 OffsetAnimationInformation;
/*
    if(OffsetModelInformation > -1)
    {
        FSeek(OffsetModelInformation);
        PapaModelInformation Model[NumberOfModels] <optimize=false>;
    };

    if(OffsetTextureInformation > -1)
    {
        FSeek(OffsetTextureInformation);
        PapaTextureInformation TextureInformation[NumberOfTextures] <optimize=false>;
    };

    if(OffsetMeshInformation > -1)
    {
        FSeek(OffsetMeshInformation);
        PapaMeshInformation MeshInformation[NumberOfMeshes] <optimize=false>;
    };

    if(OffsetMaterialInformation > -1)
    {
        FSeek(OffsetMaterialInformation);
        PapaMaterialInformation MaterialInformation[NumberOfMaterials] <optimize=false>;
    };

    if(OffsetVerticesInformation > -1)
    {
        FSeek(OffsetVerticesInformation);
        PapaVerticesInformation Vertices;
    }

    if(OffsetIndicesInformation > -1)
    {
        FSeek(OffsetIndicesInformation);
        PapaIndicesInformation Indices;
    }

    if (OffsetSkeletonInformation > -1)
    {
        FSeek(OffsetSkeletonInformation);
        PapaSkeletonInformation SkeletonInformation;
    };

    if(OffsetBonesHeader > -1)
    {
        FSeek(OffsetBonesHeader);
        PapaBonesHeader Bones(NumberOfBones);
    };*/
};

#define TF_A8R8G8B8	1
#define TF_DXT1	4
#define TF_DXT5	6

struct PapaTextureInformationHeader
{
  char Unknown1[2];
  char TextureFormat;
  char MIPS:4; //(TF_DXT1 only)
  char Unknown2:3;
  char SRGB:1 ;
  short Width;
  short Height;
  qint64 Length;
  qint64 Unknown3; //Always 128
//  char Unknown4[Length];
};
/*
struct PapaModelInformation
{
    int SkeletonData;
    int NumberOfMeshBindings;
    PapaMatrix4 Model2Scene;
    int64 OffsetMeshBinding;

    FSeek(OffsetMeshBinding);
    PapaMeshBinding MeshBinding[NumberOfMeshBindings] <optimize=false>;
};

struct PapaMeshBinding()
{
    int Unknown1;
    int SkeletonSegments;
    PapaMatrix4 Mesh2Model;
    int64 OffsetBoneMappings;

    //Counts from 0 to SkeletonSegments
    if (OffsetBoneMappings > -1) {
        FSeek(OffsetBoneMappings);
        PapaBoneMapping BoneMapping[SkeletonSegments];
    };
};

struct PapaBoneMapping
{
    short SegmentID;
};

struct PapaMeshInformation
{
    int Unknown1; //possibly 2x shorts
    int NumberOfMaterialGroups;
    int64 OffsetMaterialGroup;

    FSeek(OffsetMaterialGroup);
    PapaMaterialGroup MaterialGroup[NumberOfMaterialGroups] <optimize=false>;
};

struct PapaMaterialGroup
{
    short Unknown1;
    short MaterialIndex;
    short FirstIndex;
    short Unknown4;
    int NumberOfPrimitives;
    int PrimitiveType; //2=PRIM_Triangles
};

struct PapaMaterialInformation
{
    short Unknown1;
    short VectorParameters;
    short TexturedParameters;
    short MatrixParameters;
    int64 OffsetMaterialVectorParameter;
    int64 OffsetMaterialTextureParameters;
    int64 OffsetMaterialMatrixParameters;

    local int _Restore = FTell();
    if (OffsetMaterialVectorParameter > 0) {
        FSeek(OffsetMaterialVectorParameter);
        PapaMaterialVectorParameter MaterialVectorParameter[VectorParameters] <optimize=false>;
    };
    FSeek(_Restore);
};

struct PapaMaterialVectorParameter
{
    int Unknown1;
    float Vector[4];
    float Unknown6;
};

struct PapaMatrix3
{
    PapaMatrixRow3 Row1;
    PapaMatrixRow3 Row2;
    PapaMatrixRow3 Row3;
};

struct PapaMatrix4
{
    PapaMatrixRow4 Row1;
    PapaMatrixRow4 Row2;
    PapaMatrixRow4 Row3;
    PapaMatrixRow4 Row4;
};

struct PapaMatrixRow3
{
    float Column1;
    float Column2;
    float Column3;
};

struct PapaMatrixRow4
{
    float Column1;
    float Column2;
    float Column3;
    float Column4;
};

struct PapaVerticesInformation
{
    int VertexFormat;
    int NumberOfVertices;
    int64 SizeVerticesBlock;
    int64 OffsetVerticesBlock;

    FSeek(OffsetVerticesBlock);

    //Position3Normal3Color4TexCoord4
    if (VertexFormat == 7) {
        PapaVertex7 Vertices[NumberOfVertices];
    }

    //Position3Weights4bBones4bNormal3TexCoord2
    if (VertexFormat == 8) {
        PapaVertex8 Vertices[NumberOfVertices];
    }

    //Position3Normal3Tan3Bin3TexCoord4
    if (VertexFormat == 10) {
        PapaVertex10 Vertices[NumberOfVertices];
    }
};

//For VertexFormat 7
//Position3Normal3Color4TexCoord4
struct PapaVertex7
{
    float PositionX;
    float PositionY;
    float PositionZ;
    float NormalX;
    float NormalY;
    float NormalZ;
    byte Colour[4];
    float U;
    float V;
    float X;
    float Y;
};

//For VertexFormat 8
//Position3Weights4bBones4bNormal3TexCoord2
struct PapaVertex8
{
    float PositionX;
    float PositionY;
    float PositionZ;
    byte Weights[4];
    int BoneSegmentIndex; //should be 4 x bytes?
    float NormalX;
    float NormalY;
    float NormalZ;
    float TexCoord1;
    float TexCoord2;
};

//For VertexFormat 10
//Position3Normal3Tan3Bin3TexCoord4
struct PapaVertex10
{
    float PositionX;
    float PositionY;
    float PositionZ;
    float NormalX;
    float NormalY;
    float NormalZ;
    float TanX;
    float TanY;
    float TanZ;
    float BinX;
    float BinY;
    float BinZ;
    float X;
    float Y;
    float U;
    float V;
};

struct PapaIndicesInformation
{
    int IndexFormat;
    int NumberOfIndices;
    int64 SizeIndicesBlock;
    int64 OffsetIndicesBlock;

    //IF_UInt16
    if (IndexFormat == 0) {
        FSeek(OffsetIndicesBlock);
        PapaPrimitive0 Primitives[NumberOfIndices/3];
    };
};

struct PapaPrimitive0
{
    short VertexA;
    short VertexB;
    short VertexC;
};

struct PapaSkeletonInformation
{
    short NumberOfBones;
    short Unknown[3];
    int64 Offset;
    PapaSkeletonSegment SkeletonSegment[NumberOfBones];
};

struct PapaSkeletonSegment
{
    short BoneIndex;
    short ParentSegmentIndex;
    float TranslationX;
    float TranslationY;
    float TranslationZ;
    PapaMatrix3 ShearScale;
    PapaMatrix4 Bind2Bone;
    float BoneOffsetPositionX;  //?
    float BoneOffsetPositionY;  //?
    float BoneOffsetPositionZ;  //?
    float Unknown;              //always 1
};

struct PapaBonesHeader(int NumberOfBones)
{
    PapaBone Bones[NumberOfBones] <optimize=false>;
};
*/
struct PapaBoneHeader
{
	qint64 LengthOfBoneName;
	qint64 OffsetBoneName;
};

#endif // PAPAFILE_H
